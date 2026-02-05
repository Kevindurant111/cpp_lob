#include "pch.h"
#include "gtest/gtest.h"
#include "OrderBook.h"

// 测试：买单和卖单完全撮合成交
TEST(OrderBookTest, FullMatchTest) {
    OrderBook book;

    // 1. 挂卖单：不再传指针，传参数
    book.addOrder(Side::Sell, 100, 10);

    // 2. 挂买单
    book.addOrder(Side::Buy, 100, 10);

    // 3. 预期结果
    EXPECT_EQ(book.getOrderCount(), 0);
}

// 测试：部分成交
TEST(OrderBookTest, PartialMatchTest) {
    OrderBook book;

    book.addOrder(Side::Sell, 100, 10);
    book.addOrder(Side::Buy, 100, 4);

    EXPECT_EQ(book.getOrderCount(), 1);
    EXPECT_EQ(book.getVolumeAtPrice(Side::Sell, 100), 6);
}

TEST(OrderBookTest, SnapshotTest) {
    OrderBook book;

    book.addOrder(Side::Sell, 105, 10);
    book.addOrder(Side::Sell, 101, 10);
    book.addOrder(Side::Sell, 103, 10);
    book.addOrder(Side::Buy, 98, 5);
    book.addOrder(Side::Buy, 99, 5);

    auto snapshot = book.getSnapshot(5);

    ASSERT_FALSE(snapshot.asks.empty());
    EXPECT_EQ(snapshot.asks[0].price, 101); // 卖一价
    ASSERT_FALSE(snapshot.bids.empty());
    EXPECT_EQ(snapshot.bids[0].price, 99);  // 买一价
}

TEST(OrderBookTest, MarketOrderTest) {
    OrderBook book;

    book.addOrder(Side::Sell, 100, 10);
    book.addOrder(Side::Sell, 101, 10);

    Quantity filled = book.addMarketOrder(Side::Buy, 15);

    EXPECT_EQ(filled, 15);
    EXPECT_EQ(book.getOrderCount(), 1);
    EXPECT_EQ(book.getVolumeAtPrice(Side::Sell, 101), 5);
}

TEST(OrderBookTest, TradeCallbackTest) {
    OrderBook book;
    TradeReport lastTrade{};
    int tradeCount = 0;

    book.setTradeCallback([&](const TradeReport& report) {
        lastTrade = report;
        tradeCount++;
        });

    book.addOrder(Side::Sell, 100, 10); // Maker
    book.addOrder(Side::Buy, 100, 4);   // Taker

    EXPECT_EQ(tradeCount, 1);
    EXPECT_EQ(lastTrade.quantity, 4);
    EXPECT_EQ(lastTrade.price, 100);
}