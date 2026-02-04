#pragma once
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>

template<typename T>
class ThreadSafeObjectPool {
private:
    struct Node {
        T data;
        Node* next;
    };

    // 全局大池子：当线程局部池子不够用时才访问
    std::atomic<Node*> globalFreeList{ nullptr };

    // 内存块管理，用于最后清理
    std::vector<std::unique_ptr<Node[]>> chunks;
    std::mutex chunkMutex;

    size_t chunkSize;

public:
    ThreadSafeObjectPool(size_t initialSize = 10000) : chunkSize(initialSize) {
        expand();
    }

    // 禁止拷贝，防止内存管理混乱
    ThreadSafeObjectPool(const ThreadSafeObjectPool&) = delete;
    ThreadSafeObjectPool& operator=(const ThreadSafeObjectPool&) = delete;

    T* acquire() {
        // 尝试从全局无锁链表中弹出一个节点
        Node* oldHead = globalFreeList.load(std::memory_order_relaxed);
        while (oldHead && !globalFreeList.compare_exchange_weak(oldHead, oldHead->next,
            std::memory_order_release,
            std::memory_order_relaxed));

        if (!oldHead) {
            expand(); // 全局也干了，扩容
            return acquire();
        }

        // 使用 Placement New 在已有内存上调用构造函数
        return new (&oldHead->data) T();
    }

    void release(T* obj) {
        if (!obj) return;

        // 手动调用析构函数，但不释放内存
        obj->~T();

        // 将对象（实际是Node指针）压回全局无锁链表
        Node* node = reinterpret_cast<Node*>(obj);
        node->next = globalFreeList.load(std::memory_order_relaxed);
        while (!globalFreeList.compare_exchange_weak(node->next, node,
            std::memory_order_release,
            std::memory_order_relaxed));
    }

private:
    void expand() {
        std::lock_guard<std::mutex> lock(chunkMutex);

        auto newChunk = std::make_unique<Node[]>(chunkSize);
        for (size_t i = 0; i < chunkSize - 1; ++i) {
            newChunk[i].next = &newChunk[i + 1];
        }

        // 将新块的最后一个节点指向当前的全局头
        newChunk[chunkSize - 1].next = globalFreeList.load();
        // 更新全局头
        globalFreeList.store(&newChunk[0]);

        chunks.push_back(std::move(newChunk));
    }
};