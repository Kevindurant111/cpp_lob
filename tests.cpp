#include "gtest/gtest.h"
#include "../include/OrderBook.h"

// 这是一个会失败的测试：测试撮合逻辑
TEST(OrderBookTest, SimpleMatchTest) {
    OrderBook book;

    // 1. 先在卖盘挂一个订单：$100 卖 10 股
    Order* seller = new Order{ 1, Side::Sell, 100, 10 };
    book.addOrder(seller);

    // 2. 发送一个买单：$100 买 10 股
    Order* buyer = new Order{ 2, Side::Buy, 100, 10 };
    book.addOrder(buyer);

    // 3. 预期结果：两者撮合，卖盘 $100 的档位应该变空了
    EXPECT_FALSE(book.hasLimit(Side::Sell, 100));
    EXPECT_EQ(book.getOrderCount(), 0); // 假设成交后订单被移出索引
}