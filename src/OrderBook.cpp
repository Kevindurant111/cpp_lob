#include "OrderBook.h"
#include "Limit.h"
#include <atomic>
#include <algorithm>

OrderBook::OrderBook() : orderPool(100000), limitPool(10000) {
    // 默认回调：只打印一行简单的日志
    tradeCallback = [](const TradeReport& report) {
        printf("TRADE: Taker %llu matched Maker %llu | Qty: %u @ Price: %lld\n",
            report.takerId, report.makerId, report.quantity, report.price);
        };
}

OrderBook::~OrderBook() {
    // 1. 清空容器（此时容器内存放的是指向池化内存的指针，无需手动 delete）
    orderIndex.clear();
    bids.clear();
    asks.clear();

    // 2. 池子成员 (orderPool, limitPool) 会在这里被自动销毁
    // 它们的析构函数会释放所有预分配的大块内存。
}

/*
void OrderBook::addOrder(Order* order) {
    // 1. 尝试撮合
    if (order->side == Side::Buy) {
        match(order, asks);
    }
    else {
        match(order, bids);
    }

    // 2. 撮合后如果有剩余，挂在档位上
    if (order->quantity > 0) {
        orderIndex[order->id] = order;
        if (order->side == Side::Buy) {
            if (bids.find(order->price) == bids.end()) bids[order->price] = new Limit(order->price);
            addOrderToLimit(*bids[order->price], order);
        }
        else {
            if (asks.find(order->price) == asks.end()) asks[order->price] = new Limit(order->price);
            addOrderToLimit(*asks[order->price], order);
        }
    }
    else {
        delete order; // 已完全成交
    }
}
*/

OrderId OrderBook::addOrder(Side side, Price price, Quantity quantity) {
    Order* order = orderPool.acquire();
    static std::atomic<OrderId> nextId{ 1 };
    order->id = nextId.fetch_add(1);
    order->side = side;
    order->price = price;
    order->quantity = quantity;
    order->next = nullptr;
    order->prev = nullptr;

    if (order->side == Side::Buy) {
        match(order, asks);
    }
    else {
        match(order, bids);
    }

    if (order->quantity > 0) {
        orderIndex[order->id] = order;
        // 分开处理买卖盘，避免类型不匹配错误
        if (order->side == Side::Buy) {
            if (bids.find(order->price) == bids.end()) {
                Limit* newLimit = limitPool.acquire();
                newLimit->price = order->price;
                newLimit->totalVolume = 0;
                newLimit->orderCount = 0;
                newLimit->head = newLimit->tail = nullptr;
                bids[order->price] = newLimit;
            }
            addOrderToLimit(*bids[order->price], order);
        }
        else {
            if (asks.find(order->price) == asks.end()) {
                Limit* newLimit = limitPool.acquire();
                newLimit->price = order->price;
                newLimit->totalVolume = 0;
                newLimit->orderCount = 0;
                newLimit->head = newLimit->tail = nullptr;
                asks[order->price] = newLimit;
            }
            addOrderToLimit(*asks[order->price], order);
        }
    }
    else {
        // 如果完全成交，记得释放 Taker 订单
        orderPool.release(order);
    }
    return order->id;
}

void OrderBook::match(Order* order, std::map<Price, Limit*, std::less<Price>>& partnerMap) {
    while (order->quantity > 0 && !partnerMap.empty()) {
        auto it = partnerMap.begin();
        // 买单价格必须 >= 卖盘最低价才能成交
        if (order->price < it->first) break;

        Limit* limit = it->second;
        Order* curr = limit->head;

        while (curr && order->quantity > 0) {
            Quantity matchQty = std::min(order->quantity, curr->quantity);

            // 触发成交回报回调
            if (tradeCallback) {
                tradeCallback(TradeReport{
                    curr->id,       // Maker (已经在书上的卖单)
                    order->id,      // Taker (新进来的买单)
                    it->first,      // 成交价
                    matchQty,
                    Side::Buy       // 主动方是买方
                    });
            }

            // 更新数量
            order->quantity -= matchQty;
            curr->quantity -= matchQty;
            limit->totalVolume -= matchQty;

            if (curr->quantity == 0) {
                Order* next = curr->next;
                // 从档位链表中移除并从索引中删除
                removeOrderFromLimit(*limit, curr);
                orderIndex.erase(curr->id);

                // --- 核心修改：回收订单内存到池子 ---
                orderPool.release(curr);

                curr = next;
            }
            else {
                break; // 当前卖单没被吃完，买单已耗尽
            }
        }

        // 如果档位空了，移除档位并回收 Limit 内存
        if (limit->orderCount == 0) {
            partnerMap.erase(it);
            limitPool.release(limit);
        }
    }
}

void OrderBook::match(Order* order, std::map<Price, Limit*, std::greater<Price>>& partnerMap) {
    while (order->quantity > 0 && !partnerMap.empty()) {
        auto it = partnerMap.begin();
        // 卖单价格必须 <= 买盘最高价才能成交
        if (order->price > it->first) break;

        Limit* limit = it->second;
        Order* curr = limit->head;

        while (curr && order->quantity > 0) {
            Quantity matchQty = std::min(order->quantity, curr->quantity);

            if (tradeCallback) {
                tradeCallback(TradeReport{
                    curr->id,       // Maker (已经在书上的买单)
                    order->id,      // Taker (新进来的卖单)
                    it->first,
                    matchQty,
                    Side::Sell      // 主动方是卖方
                    });
            }

            order->quantity -= matchQty;
            curr->quantity -= matchQty;
            limit->totalVolume -= matchQty;

            if (curr->quantity == 0) {
                Order* next = curr->next;
                removeOrderFromLimit(*limit, curr);
                orderIndex.erase(curr->id);

                // --- 核心修改：回收订单内存到池子 ---
                orderPool.release(curr);

                curr = next;
            }
            else {
                break;
            }
        }

        if (limit->orderCount == 0) {
            partnerMap.erase(it);
            limitPool.release(limit);
        }
    }
}

size_t OrderBook::getOrderCount() const {
    return orderIndex.size();
}

bool OrderBook::hasLimit(Side side, Price price) const {
    return (side == Side::Buy) ? bids.count(price) > 0 : asks.count(price) > 0;
}

Quantity OrderBook::getVolumeAtPrice(Side side, Price price) const {
    if (side == Side::Buy) {
        auto it = bids.find(price);
        return (it != bids.end()) ? it->second->totalVolume : 0;
    }
    else {
        auto it = asks.find(price);
        return (it != asks.end()) ? it->second->totalVolume : 0;
    }
}

void OrderBook::cancelOrder(OrderId orderId) {
    auto it = orderIndex.find(orderId);
    if (it == orderIndex.end()) return;

    Order* order = it->second;
    Price price = order->price;

    if (order->side == Side::Buy) {
        auto lit = bids.find(price);
        if (lit != bids.end()) {
            Limit* limit = lit->second;
            limit->totalVolume -= order->quantity;
            removeOrderFromLimit(*limit, order);
            if (limit->orderCount == 0) {
                limitPool.release(limit);
                bids.erase(lit);
            }
        }
    }
    else {
        auto lit = asks.find(price);
        if (lit != asks.end()) {
            Limit* limit = lit->second;
            limit->totalVolume -= order->quantity;
            removeOrderFromLimit(*limit, order);
            if (limit->orderCount == 0) {
                limitPool.release(limit);
                asks.erase(lit);
            }
        }
    }

    orderIndex.erase(it);
    orderPool.release(order);
}

MarketSnapshot OrderBook::getSnapshot(int depth) const {
    MarketSnapshot snapshot;

    // 提取买盘 (Bids) - 已经是降序
    int b_count = 0;
    for (auto const& [price, limit] : bids) {
        if (b_count >= depth) break;
        snapshot.bids.push_back({ price, limit->totalVolume });
        b_count++;
    }

    // 提取卖盘 (Asks) - 已经是升序
    int a_count = 0;
    for (auto const& [price, limit] : asks) {
        if (a_count >= depth) break;
        snapshot.asks.push_back({ price, limit->totalVolume });
        a_count++;
    }

    return snapshot;
}

Quantity OrderBook::addMarketOrder(Side side, Quantity quantity) {
    if (quantity <= 0) return 0;

    Quantity originalQuantity = quantity;

    // 构造一个价格为“无穷大”或“无穷小”的临时订单
    // 这样它就能匹配对手盘上的任何价格
    Price marketPrice = (side == Side::Buy) ?
        std::numeric_limits<Price>::max() :
        0;

    // 临时创建一个撮合对象
    // 注意：ID 设为 0，因为市价单如果不成交也不会入册挂单
    Order tempOrder{ 0, side, marketPrice, quantity };

    if (side == Side::Buy) {
        match(&tempOrder, asks);
    }
    else {
        match(&tempOrder, bids);
    }

    // 返回实际成交的数量
    return originalQuantity - tempOrder.quantity;
}