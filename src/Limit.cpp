#include "../include/Limit.h"

void addOrderToLimit(Limit& limit, Order* order) {
    // 1. 更新该价格档位的统计信息
    limit.totalVolume += order->quantity; // 该价格的总报单量增加
    limit.orderCount++;                  // 该价格的订单总数加 1

    // 2. 将订单放入双向链表
    if (limit.head == nullptr) {
        // 【情况 A】这个价格档位目前是空的（它是第一个报单）
        limit.head = order;      // 头指针指向它
        limit.tail = order;      // 尾指针也指向它
        order->prev = nullptr;   // 前面没人
        order->next = nullptr;   // 后面也没人
    }
    else {
        // 【情况 B】已经有人在排队了，新订单必须排在最后面（FIFO 核心）
        order->prev = limit.tail; // 新订单的“前任”是现在的队尾
        limit.tail->next = order; // 让现在的队尾指向新订单，把它接在后面
        limit.tail = order;       // 更新队尾指针，现在新订单成了新的队尾
        order->next = nullptr;    // 它是最后一名，后面没人
    }
}

void removeOrderFromLimit(Limit& limit, Order* order) {
    if (order->prev) order->prev->next = order->next;
    if (order->next) order->next->prev = order->prev;

    if (limit.head == order) limit.head = order->next;
    if (limit.tail == order) limit.tail = order->prev;

    limit.orderCount--;
    // 注意：这里不要再写 limit.totalVolume -= ...
    // 因为成交导致的移除量已经在 match 中减过了
    // 撤单导致的移除量我们在 cancelOrder 中手动处理了
}