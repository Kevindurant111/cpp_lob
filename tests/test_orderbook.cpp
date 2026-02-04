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

TEST(OrderBookTest, MarketOrderTest) {
    OrderBook book;

    // 先在不同价格挂上卖单
    book.addOrder(new Order{ 1, Side::Sell, 100, 10 }); // 卖一 10股
    book.addOrder(new Order{ 2, Side::Sell, 101, 10 }); // 卖二 10股

    // 发送一个 15 股的买入市价单
    Quantity filled = book.addMarketOrder(Side::Buy, 15);

    EXPECT_EQ(filled, 15);
    EXPECT_EQ(book.getOrderCount(), 1); // 卖一应该被吃光了，只剩卖二
    EXPECT_EQ(book.getVolumeAtPrice(Side::Sell, 101), 5); // 卖二还剩 5股
}

TEST(OrderBookTest, TradeCallbackTest) {
    OrderBook book;
    TradeReport lastTrade{};
    int tradeCount = 0;

    // 设置一个简单的回调，把成交信息记录下来
    book.setTradeCallback([&](const TradeReport& report) {
        lastTrade = report;
        tradeCount++;
        });

    // 1. 先挂一个卖单 (Maker)
    book.addOrder(new Order{ 101, Side::Sell, 100, 10 });

    // 2. 发送一个买单去匹配 (Taker)
    book.addOrder(new Order{ 102, Side::Buy, 100, 4 });

    // 验证回调是否被触发
    EXPECT_EQ(tradeCount, 1);
    EXPECT_EQ(lastTrade.makerId, 101);
    EXPECT_EQ(lastTrade.takerId, 102);
    EXPECT_EQ(lastTrade.quantity, 4);
    EXPECT_EQ(lastTrade.price, 100);
}