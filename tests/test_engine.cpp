#include <gtest/gtest.h>
#include "MatchingEngine.hpp"
#include "MemoryArena.hpp"
#include "TradeTicket.hpp"

using namespace exchange;

// =============================================================
// MARKET ORDER TESTS
// =============================================================

TEST(MarketOrderTest, BuyMarketOrderFillsBestAsk) {
    MatchingEngine engine;
    // Place a sell limit order at price 300, qty 100
    engine.submitLimitTicket(1, false, 100, 300);
    // Place a buy market order for qty 100
    engine.submitMarketTicket(2, true, 100);
    // The sell order should have been fully consumed
    EXPECT_EQ(engine.executedTicketsCount, 1);
}

TEST(MarketOrderTest, SellMarketOrderFillsBestBid) {
    MatchingEngine engine;
    // Place a buy limit order at price 300, qty 100
    engine.submitLimitTicket(1, true, 100, 300);
    // Place a sell market order for qty 100
    engine.submitMarketTicket(2, false, 100);
    EXPECT_EQ(engine.executedTicketsCount, 1);
}

TEST(MarketOrderTest, LargeMarketOrderConsumesBothOrders) {
    MatchingEngine engine;
    // Two sell orders at the same price level
    engine.submitLimitTicket(1, false, 50, 300);
    engine.submitLimitTicket(2, false, 50, 300);
    // Market order for 100 should consume both
    engine.submitMarketTicket(3, true, 100);
    EXPECT_EQ(engine.executedTicketsCount, 2);
}

// =============================================================
// LIMIT ORDER TESTS
// =============================================================

TEST(LimitOrderTest, LimitOrderRestsWhenNoCrossableOrder) {
    MatchingEngine engine;
    engine.submitLimitTicket(1, true, 100, 300);
    // No match exists — order should rest in the book
    EXPECT_NE(engine.getTopBid(), nullptr);
    EXPECT_EQ(engine.getTopBid()->getLimitPrice(), 300);
}

TEST(LimitOrderTest, BestBidAndAskReflectSubmittedOrders) {
    MatchingEngine engine;
    engine.submitLimitTicket(1, true, 100, 299);   // Bid at 299
    engine.submitLimitTicket(2, false, 100, 301);  // Ask at 301
    EXPECT_EQ(engine.getTopBid()->getLimitPrice(), 299);
    EXPECT_EQ(engine.getBestAsk()->getLimitPrice(), 301);
}

TEST(LimitOrderTest, MultipleOrdersAtSamePriceLevelAggregateVolume) {
    MatchingEngine engine;
    // Three buy orders all at price 300
    engine.submitLimitTicket(1, true, 50, 300);
    engine.submitLimitTicket(2, true, 75, 300);
    engine.submitLimitTicket(3, true, 25, 300);
    EXPECT_EQ(engine.getTopBid()->getTotalVolume(), 150);
    EXPECT_EQ(engine.getTopBid()->getSize(), 3);
}

// =============================================================
// ORDER CANCELLATION TESTS
// =============================================================

TEST(CancelOrderTest, CancelReducesVolumeAtPriceLevel) {
    MatchingEngine engine;
    engine.submitLimitTicket(1, true, 100, 300);
    engine.submitLimitTicket(2, true, 50, 300);
    engine.cancelLimitTicket(1);
    EXPECT_EQ(engine.getTopBid()->getTotalVolume(), 50);
    EXPECT_EQ(engine.getTopBid()->getSize(), 1);
}

TEST(CancelOrderTest, CancelLastOrderRemovesPriceLevel) {
    MatchingEngine engine;
    engine.submitLimitTicket(1, true, 100, 300);
    engine.cancelLimitTicket(1);
    // The price level should no longer exist in the book
    EXPECT_EQ(engine.getTopBid(), nullptr);
}

// =============================================================
// STOP ORDER TESTS
// =============================================================

TEST(StopOrderTest, StopSellOrderExistsInStopTree) {
    MatchingEngine engine;
    // Seed the book so stop prices have a valid reference
    engine.submitLimitTicket(1, true, 100, 290);   // Best bid = 290
    engine.submitLimitTicket(2, false, 100, 310);  // Best ask = 310
    // Stop sell below current bid (trigger < bid, waits for price to drop)
    engine.submitStopTicket(3, false, 50, 280);
    EXPECT_NE(engine.getHighestStopSell(), nullptr);
}

TEST(StopOrderTest, StopBuyOrderExistsInStopTree) {
    MatchingEngine engine;
    engine.submitLimitTicket(1, true, 100, 290);
    engine.submitLimitTicket(2, false, 100, 310);
    // Stop buy above current ask (trigger > ask, waits for price to rise)
    engine.submitStopTicket(3, true, 50, 320);
    EXPECT_NE(engine.getLowestStopBuy(), nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
