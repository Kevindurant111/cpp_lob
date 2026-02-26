#include "pch.h"
#include <chrono>
#include <vector>
#include "OrderBook.h"
#include <iostream>

void PerformanceBenchmark() {
    OrderBook book;
    const int iterations = 1000000; // 100 万笔订单

    // 禁用回调以排除 IO 干扰
    book.setTradeCallback(nullptr);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        // 模拟交替买卖，产生大量撮合和内存申请/释放
        if (i % 2 == 0) {
            book.addOrder(Side::Sell, 100, 10);
        }
        else {
            book.addOrder(Side::Buy, 100, 10);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    double tps = iterations / diff.count();
    std::cout << "Processed " << iterations << " orders in " << diff.count() << " seconds." << std::endl;
    std::cout << "Throughput: " << tps << " orders/sec (TPS)" << std::endl;
    std::cout << "Avg Latency: " << (diff.count() * 1e9 / iterations) << " ns/order" << std::endl;
}