#pragma once
#include <cstdint>
#include <vector>

enum class Side : uint8_t {
    Buy,
    Sell
};

using OrderId = uint64_t;
using Price = int64_t;   // 量化通常用整数表示价格（如：真实价格 * 10000），避免浮点误差
using Quantity = uint32_t;

struct Level {
    Price price;
    Quantity volume;
};

struct MarketSnapshot {
    std::vector<Level> bids;
    std::vector<Level> asks;
};