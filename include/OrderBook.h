#pragma once
#include <map>
#include <unordered_map>
#include "Limit.h"
#include "Order.h"
#include "ObjectPool.h"
#include "Types.h"
#include <functional>

// 定义回调函数的类型
using TradeCallback = std::function<void(const TradeReport&)>;

class OrderBook {
private:
    // --- 内存池定义在此 ---
    // 建议预分配足够大的空间，例如 10 万个对象
    ThreadSafeObjectPool<Order> orderPool;
    ThreadSafeObjectPool<Limit> limitPool;

    TradeCallback tradeCallback; // 存储回调

    // 卖盘：价格升序
    std::map<Price, Limit*> asks;

    // 买盘：价格降序
    std::map<Price, Limit*, std::greater<Price>> bids;

    // 全局订单索引
    std::unordered_map<OrderId, Order*> orderIndex;

public:
    OrderBook();
    ~OrderBook();

    // 核心撮合接口
    OrderId addOrder(Side side, Price price, Quantity quantity);

    // 撤单接口
    void cancelOrder(OrderId orderId);

    // 辅助查询接口
    size_t getOrderCount() const;
    bool hasLimit(Side side, Price price) const;
    Quantity getVolumeAtPrice(Side side, Price price) const;

    // 获取盘口快照，depth 默认为 5
    MarketSnapshot getSnapshot(int depth = 5) const;

    // 执行市价单，返回实际成交的总量
    Quantity addMarketOrder(Side side, Quantity quantity);

    // 设置回调接口
    void setTradeCallback(TradeCallback callback) {
        tradeCallback = callback;
    }
private:
    // 内部私有撮合逻辑，减少代码重复
    void match(Order* order, std::map<Price, Limit*, std::less<Price>>& partnerMap);
    void match(Order* order, std::map<Price, Limit*, std::greater<Price>>& partnerMap);
};