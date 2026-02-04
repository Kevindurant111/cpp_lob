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

// 统一的撮合逻辑：针对入场订单（order）在对手盘（partnerMap）中寻找匹配
// 如果入场的是买单，partnerMap 就是卖盘 (Asks)，按价格升序排列
void OrderBook::match(Order* order, std::map<Price, Limit*, std::less<Price>>& partnerMap) {

    // 只要入场订单还有剩余数量，且对手盘不为空，就继续撮合
    while (order->quantity > 0 && !partnerMap.empty()) {

        // 获取对手盘中价格最优的档位（对于卖盘是最低价，对于买盘是最高价）
        auto it = partnerMap.begin();

        // 价格检查：如果入场买单价格 < 对手盘最低卖价，无法成交，跳出循环
        // 注意：如果是撮合卖单，这里的判断逻辑需要根据传入的 partnerMap 排序规则（大于/小于）相应调整
        if (order->price < it->first) break;

        // 获取该价格档位（Limit Node）对应的订单链表
        Limit* limit = it->second;
        Order* curr = limit->head; // 从该档位时间最早的订单开始撮合（时间优先）

        // 遍历当前价格档位下的所有订单
        while (curr && order->quantity > 0) {
            // 计算本次能够成交的数量：取入场单剩余量和对手单存量的最小值
            Quantity matchQty = std::min(order->quantity, curr->quantity);

            // --- 核心撮合步骤 ---
            order->quantity -= matchQty;      // 减少入场订单的剩余数量
            curr->quantity -= matchQty;       // 减少对手盘订单的剩余数量
            limit->totalVolume -= matchQty;   // 同步更新该价格档位的总报单量

            // 如果对手盘的这个订单被完全吃掉（成交完）
            if (curr->quantity == 0) {
                Order* next = curr->next;      // 暂存下一个订单指针

                // 从该价格档位的双向链表中移除当前订单
                removeOrderFromLimit(*limit, curr);

                // 从全局订单索引中删除该订单 ID（防止后续通过 ID 还能查到已成交单）
                orderIndex.erase(curr->id);

                // curr = next; 指向下一个对手单，继续处理当前档位
                curr = next;
            }
            else {
                // 如果对手盘订单还没被吃完，说明入场订单已经消耗光了
                // 此时直接跳出内层循环
                break;
            }
        }

        // 检查该价格档位是否已经空了（订单数量为 0）
        if (limit->orderCount == 0) {
            // 释放该价格档位的内存（Limit 对象）
            delete limit;
            // 从 map 结构中移除该价格节点
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

// 撤单函数：通过订单 ID 撤销一个尚未完全成交的挂单
void OrderBook::cancelOrder(OrderId orderId) {
    // 1. 在全局哈希表 orderIndex 中查找该订单 ID
    auto it = orderIndex.find(orderId);

    // 如果找不到该 ID，说明订单已成交、已撤销或根本不存在，直接返回
    if (it == orderIndex.end()) return;

    // 2. 获取订单对象的指针和该订单的挂单价格
    Order* order = it->second;
    Price price = order->price;

    // 3. 根据订单的方向（买或卖）进入不同的处理逻辑
    if (order->side == Side::Buy) {
        // --- 处理买单撤单 ---
        // 在买盘 (bids) 中找到该价格对应的档位 (Limit 对象)
        auto lit = bids.find(price);
        if (lit != bids.end()) {
            // 从该价格档位的总报单量中减去该订单“剩余”的数量
            lit->second->totalVolume -= order->quantity;

            // 调用辅助函数，将 order 从该档位的双向链表中断开连接
            removeOrderFromLimit(*(lit->second), order);

            // 如果该价格档位下的订单数为 0，说明该价格已无任何挂单
            if (lit->second->orderCount == 0) {
                delete lit->second; // 释放 Limit 对象的内存
                bids.erase(lit);    // 从买盘 map 中移除该价格节点
            }
        }
    }
    else {
        // --- 处理卖单撤单 ---
        // 在卖盘 (asks) 中找到该价格对应的档位
        auto lit = asks.find(price);
        if (lit != asks.end()) {
            // 同步减少卖盘该档位的总报单量
            lit->second->totalVolume -= order->quantity;

            // 从卖盘链表中移除该订单
            removeOrderFromLimit(*(lit->second), order);

            // 如果档位空了，清理内存并移除价格节点
            if (lit->second->orderCount == 0) {
                delete lit->second;
                asks.erase(lit);
            }
        }
    }

    // 4. 最后从全局订单索引中移除该 ID，确保该订单彻底从系统消失
    orderIndex.erase(it);

    // 注意：如果是生产环境，此处通常需要 delete order; 释放订单内存
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