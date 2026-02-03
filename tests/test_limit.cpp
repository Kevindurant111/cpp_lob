#include "pch.h"
#include "gtest/gtest.h"
#include "Limit.h"    // 确保包含你实现逻辑的头文件
#include "Order.h"
#include "Types.h"

// 测试：添加单个订单到价格档位
TEST(LimitTest, AddSingleOrder) {
    Limit limit(100);
    Order* o1 = new Order{ 1, Side::Buy, 100, 10 };

    addOrderToLimit(limit, o1);

    EXPECT_EQ(limit.orderCount, 1);
    EXPECT_EQ(limit.totalVolume, 10);
    EXPECT_EQ(limit.head, o1);
    EXPECT_EQ(limit.tail, o1);

    delete o1;
}

// 测试：删除中间的订单（链表架空逻辑）
TEST(LimitTest, RemoveMiddleOrder) {
    Limit limit(100);
    Order* o1 = new Order{ 1, Side::Buy, 100, 10 };
    Order* o2 = new Order{ 2, Side::Buy, 100, 10 };
    Order* o3 = new Order{ 3, Side::Buy, 100, 10 };

    addOrderToLimit(limit, o1);
    addOrderToLimit(limit, o2);
    addOrderToLimit(limit, o3);

    removeOrderFromLimit(limit, o2);

    EXPECT_EQ(limit.orderCount, 2);
    EXPECT_EQ(o1->next, o3);
    EXPECT_EQ(o3->prev, o1);
    EXPECT_EQ(limit.head, o1);
    EXPECT_EQ(limit.tail, o3);

    delete o1; delete o2; delete o3;
}