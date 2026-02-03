#pragma once
#include "Types.h"  

struct Order {
    OrderId id;
    Side side;
    Price price;
    Quantity quantity;
    // 用于在双向链表中快速移动
    Order* next = nullptr;
    Order* prev = nullptr;
};