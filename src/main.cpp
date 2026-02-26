#include "OrderBook.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>

void RunBenchmark() {
    OrderBook book;
    const int iterations = 1000000;
    const int warmup_iters = 100000; // 预热次数

    // 提前分配内存，防止测试中途 vector 扩容干扰结果
    std::vector<long long> latencies;
    latencies.reserve(iterations);

    book.setTradeCallback(nullptr);

    // 1. Warm-up 阶段
    std::cout << "Warming up..." << std::endl;
    for (int i = 0; i < warmup_iters; ++i) {
        if (i % 2 == 0) book.addOrder(Side::Sell, 100 + (i % 5), 10);
        else book.addOrder(Side::Buy, 100 + (i % 5), 10);
    }

    // 2. 正式测试阶段
    std::cout << "Benchmarking & Collecting Latency Data..." << std::endl;

    auto total_start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        // 高精度计时开始
        auto start = std::chrono::high_resolution_clock::now();

        if (i % 2 == 0) {
            book.addOrder(Side::Sell, 100 + (i % 5), 10);
        }
        else {
            book.addOrder(Side::Buy, 100 + (i % 5), 10);
        }

        auto end = std::chrono::high_resolution_clock::now();
        // 高精度计时结束

        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        latencies.push_back(duration);
    }

    auto total_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total_diff = total_end - total_start;

    // --- 统计分析 ---
    std::sort(latencies.begin(), latencies.end());

    long long sum = std::accumulate(latencies.begin(), latencies.end(), 0LL);
    double avg = static_cast<double>(sum) / iterations;

    std::cout << "\n================ PERFORMANCE REPORT ================" << std::endl;
    std::cout << "Total Orders: " << iterations << std::endl;
    std::cout << "Throughput:   " << std::fixed << std::setprecision(2)
        << (iterations / total_diff.count()) / 1e6 << " Million orders/s" << std::endl;

    std::cout << "\n--- Latency Distribution (Time per Order) ---" << std::endl;
    std::cout << "Average:      " << std::setprecision(2) << avg << " ns" << std::endl;
    std::cout << "Min:          " << latencies.front() << " ns" << std::endl;
    std::cout << "P50 (Median): " << latencies[iterations * 0.50] << " ns" << std::endl;
    std::cout << "P90:          " << latencies[iterations * 0.90] << " ns" << std::endl;
    std::cout << "P99:          " << latencies[iterations * 0.99] << " ns" << std::endl;
    std::cout << "P99.9:        " << latencies[iterations * 0.999] << " ns" << std::endl;
    std::cout << "Max:          " << latencies.back() << " ns" << std::endl;
    std::cout << "====================================================" << std::endl;
}

int main() {
    // 确保以 Release 模式运行
#ifdef _DEBUG
    std::cout << "WARNING: You are running in DEBUG mode. Performance will be poor!" << std::endl;
#endif

    RunBenchmark();

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
    return 0;
}