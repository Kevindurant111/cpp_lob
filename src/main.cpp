#include "OrderBook.h"
#include <iostream>
#include <chrono>

// 将刚才的测试逻辑封装
void RunBenchmark() {
    OrderBook book;
    const int iterations = 1000000;

    // 确保不打印日志，否则测的是打印速度而不是撮合速度
    book.setTradeCallback(nullptr);

    std::cout << "Benchmarking..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        if (i % 2 == 0) book.addOrder(Side::Sell, 100 + (i % 5), 10);
        else book.addOrder(Side::Buy, 100 + (i % 5), 10);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "TPS: " << (iterations / diff.count()) / 1e6 << " Million/s" << std::endl;
}

int main() {
    RunBenchmark();
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    return 0;
}