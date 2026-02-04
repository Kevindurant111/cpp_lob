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

TEST(OrderBookTest, SnapshotTest) {
    OrderBook book;

    // 插入几个不同价格的卖单
    book.addOrder(new Order{ 1, Side::Sell, 105, 10 });
    book.addOrder(new Order{ 2, Side::Sell, 101, 10 });
    book.addOrder(new Order{ 3, Side::Sell, 103, 10 });

    // 插入几个不同价格的买单
    book.addOrder(new Order{ 4, Side::Buy, 98, 5 });
    book.addOrder(new Order{ 5, Side::Buy, 99, 5 });

    auto snapshot = book.getSnapshot(5);

    // 验证卖一价（最低卖价）应该是 101
    ASSERT_FALSE(snapshot.asks.empty());
    EXPECT_EQ(snapshot.asks[0].price, 101);

    // 验证买一价（最高买价）应该是 99
    ASSERT_FALSE(snapshot.bids.empty());
    EXPECT_EQ(snapshot.bids[0].price, 99);

    // 验证卖盘数量
    EXPECT_EQ(snapshot.asks.size(), 3);
}