#include "pch.h"
#include "gtest/gtest.h"
#include "OrderBook.h"

// 测试：买单和卖单完全撮合成交
TEST(OrderBookTest, FullMatchTest) {
    OrderBook book;

    // 1. 先挂一个卖单：100元 卖 10股
    Order* seller = new Order{ 1, Side::Sell, 100, 10 };
    book.addOrder(seller);

    // 2. 进来一个买单：100元 买 10股
    Order* buyer = new Order{ 2, Side::Buy, 100, 10 };
    book.addOrder(buyer);

    // 3. 预期结果：全额成交，订单索引变空
    EXPECT_EQ(book.getOrderCount(), 0);
    EXPECT_FALSE(book.hasLimit(Side::Sell, 100));
}

// 测试：部分成交
TEST(OrderBookTest, PartialMatchTest) {
    OrderBook book;

    // 1. 卖盘挂 10股
    book.addOrder(new Order{ 1, Side::Sell, 100, 10 });

    // 2. 买单只要买 4股
    book.addOrder(new Order{ 2, Side::Buy, 100, 4 });

    // 3. 预期：卖盘还剩 6股
    EXPECT_EQ(book.getOrderCount(), 1);
    EXPECT_EQ(book.getVolumeAtPrice(Side::Sell, 100), 6);
}