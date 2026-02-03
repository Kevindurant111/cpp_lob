#pragma once
#include "Order.h"
#include "Types.h"

struct Limit {
    Price price;
    Quantity totalVolume = 0;
    uint32_t orderCount = 0;
    Order* head = nullptr;
    Order* tail = nullptr;

    Limit(Price p) : price(p) {}
};

void addOrderToLimit(Limit& limit, Order* order);
void removeOrderFromLimit(Limit& limit, Order* order);