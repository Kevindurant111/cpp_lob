#pragma once
#include "Order.h"
#include "Types.h"

struct Limit {
    Price price;
    Quantity totalVolume = 0;
    uint32_t orderCount = 0;
    Order* head = nullptr;
    Order* tail = nullptr;

    Limit() : price(0), totalVolume(0), orderCount(0), head(nullptr), tail(nullptr) {}
    Limit(Price p) : price(p), totalVolume(0), orderCount(0), head(nullptr), tail(nullptr) {}
};

void addOrderToLimit(Limit& limit, Order* order);
void removeOrderFromLimit(Limit& limit, Order* order);