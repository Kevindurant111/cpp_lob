#include "OrderBook.h"
#include "Limit.h"
#include <algorithm>

OrderBook::~OrderBook() {
    for (auto& pair : bids) delete pair.second;
    for (auto& pair : asks) delete pair.second;
}

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

// 统一的撮合逻辑：针对买单撮合卖盘 (partnerMap 是 asks)
void OrderBook::match(Order* order, std::map<Price, Limit*, std::less<Price>>& partnerMap) {
    while (order->quantity > 0 && !partnerMap.empty()) {
        auto it = partnerMap.begin();
        if (order->price < it->first) break;

        Limit* limit = it->second;
        Order* curr = limit->head;
        while (curr && order->quantity > 0) {
            Quantity matchQty = std::min(order->quantity, curr->quantity);

            // 核心修正：同步减少三个维度的数量
            order->quantity -= matchQty;
            curr->quantity -= matchQty;
            limit->totalVolume -= matchQty;

            if (curr->quantity == 0) {
                Order* next = curr->next;
                removeOrderFromLimit(*limit, curr);
                orderIndex.erase(curr->id);
                // delete curr; // 如果是生产环境，在这里释放成交订单内存
                curr = next;
            }
            else {
                break; // 当前订单没吃完，新单已耗尽
            }
        }
        if (limit->orderCount == 0) {
            delete limit;
            partnerMap.erase(it);
        }
    }
}

// 统一的撮合逻辑：针对卖单撮合买盘 (partnerMap 是 bids)
void OrderBook::match(Order* order, std::map<Price, Limit*, std::greater<Price>>& partnerMap) {
    while (order->quantity > 0 && !partnerMap.empty()) {
        auto it = partnerMap.begin();
        if (order->price > it->first) break;

        Limit* limit = it->second;
        Order* curr = limit->head;
        while (curr && order->quantity > 0) {
            Quantity matchQty = std::min(order->quantity, curr->quantity);

            order->quantity -= matchQty;
            curr->quantity -= matchQty;
            limit->totalVolume -= matchQty;

            if (curr->quantity == 0) {
                Order* next = curr->next;
                removeOrderFromLimit(*limit, curr);
                orderIndex.erase(curr->id);
                // delete curr;
                curr = next;
            }
            else {
                break;
            }
        }
        if (limit->orderCount == 0) {
            delete limit;
            partnerMap.erase(it);
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
            // 撤单时需要减去该订单剩余的全部挂单量
            lit->second->totalVolume -= order->quantity;
            removeOrderFromLimit(*(lit->second), order);
            if (lit->second->orderCount == 0) {
                delete lit->second;
                bids.erase(lit);
            }
        }
    }
    else {
        auto lit = asks.find(price);
        if (lit != asks.end()) {
            lit->second->totalVolume -= order->quantity;
            removeOrderFromLimit(*(lit->second), order);
            if (lit->second->orderCount == 0) {
                delete lit->second;
                asks.erase(lit);
            }
        }
    }
    orderIndex.erase(it);
}