#include "order_book.hpp"
#include "matching_engine.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <functional>
#include <random>
#include <unordered_set>
#include <chrono>
#include <thread>
#include <atomic>

using namespace trading;

// Simple test framework
class AdvancedTestSuite {
public:
    using TestCase = std::function<void()>;

    void add_test(const std::string& name, TestCase test) {
        tests_.push_back({name, test});
    }

    void run_all() {
        std::cout << "Running " << tests_.size() << " advanced tests..." << std::endl;

        int passed = 0;
        for (const auto& [name, test] : tests_) {
            std::cout << "Test: " << name << " - ";
            try {
                test();
                std::cout << "PASSED" << std::endl;
                passed++;
            } catch (const std::exception& e) {
                std::cout << "FAILED: " << e.what() << std::endl;
            } catch (...) {
                std::cout << "FAILED with unknown exception" << std::endl;
            }
        }

        std::cout << "\nAdvanced Test Results: " << passed << "/" << tests_.size() << " tests passed" << std::endl;
    }

private:
    std::vector<std::pair<std::string, TestCase>> tests_;
};

// Helper function to assert with a message
void assert_with_message(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// Helper to generate timestamps
uint64_t get_timestamp() {
    static uint64_t timestamp = 1000000; // Start with a non-zero value
    return timestamp++;
}

// Helper for random order generation
class RandomOrderGenerator {
public:
    RandomOrderGenerator(uint64_t seed = 42) : generator_(seed) {}

    std::shared_ptr<Order> random_limit_order(OrderSide side, const std::string& symbol) {
        static int order_id = 0;
        std::string id = "TEST" + std::to_string(++order_id);
        uint64_t size = size_dist_(generator_);
        double price;

        if (side == OrderSide::Buy) {
            price = 90.0 + (price_dist_(generator_) / 10.0); // 90.0 - 110.0
        } else {
            price = 90.0 + (price_dist_(generator_) / 10.0); // 90.0 - 110.0
        }

        return std::make_shared<Order>(id, side, symbol, size, price, get_timestamp());
    }

    std::shared_ptr<Order> random_market_order(OrderSide side, const std::string& symbol) {
        static int order_id = 0;
        std::string id = "MKT" + std::to_string(++order_id);
        uint64_t size = size_dist_(generator_);

        return std::make_shared<Order>(id, side, symbol, size, get_timestamp());
    }

private:
    std::mt19937_64 generator_;
    std::uniform_int_distribution<uint64_t> size_dist_{1, 1000};
    std::uniform_int_distribution<int> price_dist_{0, 200}; // 0.0 - 20.0 after division
};

int main() {
    AdvancedTestSuite tests;
    RandomOrderGenerator generator;

    // Test 1: Large Order Partial Fills
    tests.add_test("Large Order Partial Fills", [&]() {
        OrderBook book("TEST");

        // Add several small sell orders at different prices
        auto sell1 = std::make_shared<Order>("SELL1", OrderSide::Sell, "TEST", 100, 10.0, get_timestamp());
        auto sell2 = std::make_shared<Order>("SELL2", OrderSide::Sell, "TEST", 200, 11.0, get_timestamp());
        auto sell3 = std::make_shared<Order>("SELL3", OrderSide::Sell, "TEST", 300, 12.0, get_timestamp());

        book.add_order(sell1);
        book.add_order(sell2);
        book.add_order(sell3);

        // Place a large buy order that will match all these sells
        auto buy = std::make_shared<Order>("BUY1", OrderSide::Buy, "TEST", 1000, 15.0, get_timestamp());
        auto trades = book.match_order(buy);

        // Check if the order was filled against all three sells (partial fill)
        assert_with_message(trades.size() == 3, "Expected 3 trades");
        assert_with_message(trades[0].order_id_sell == "SELL1", "Expected first trade with SELL1");
        assert_with_message(trades[1].order_id_sell == "SELL2", "Expected second trade with SELL2");
        assert_with_message(trades[2].order_id_sell == "SELL3", "Expected third trade with SELL3");

        // Check if sizes match
        assert_with_message(trades[0].size == 100, "Expected trade size 100");
        assert_with_message(trades[1].size == 200, "Expected trade size 200");
        assert_with_message(trades[2].size == 300, "Expected trade size 300");

        // Check if the buy order was added to the book with remaining size
        assert_with_message(buy->filled_size == 600, "Expected 600 filled");
        assert_with_message(buy->remaining_size() == 400, "Expected 400 remaining");

        // The buy order should be added to the book since it's not fully filled
        book.add_order(buy);
        assert_with_message(book.best_bid() == 15.0, "Expected best bid 15.0");
    });

    // Test 2: Market Order Behavior with Empty Book
    tests.add_test("Market Order with Empty Book", [&]() {
        OrderBook book("TEST");

        // Place a market buy order against an empty book
        auto buy = std::make_shared<Order>("BUY1", OrderSide::Buy, "TEST", 100, get_timestamp());
        auto trades = book.match_order(buy);

        // No trades should occur without any sell orders
        assert_with_message(trades.empty(), "Expected no trades");

        // Place a market sell order against an empty book
        auto sell = std::make_shared<Order>("SELL1", OrderSide::Sell, "TEST", 100, get_timestamp());
        trades = book.match_order(sell);

        // No trades should occur without any buy orders
        assert_with_message(trades.empty(), "Expected no trades");
    });

    // Test 3: Order ID Uniqueness
    tests.add_test("Order ID Uniqueness", [&]() {
        OrderBook book("TEST");

        // Add an order with ID "UNIQUE"
        auto order1 = std::make_shared<Order>("UNIQUE", OrderSide::Buy, "TEST", 100, 10.0, get_timestamp());
        book.add_order(order1);

        // Try to add another order with the same ID
        auto order2 = std::make_shared<Order>("UNIQUE", OrderSide::Buy, "TEST", 200, 11.0, get_timestamp());

        // We should still be able to add it (no check for uniqueness in the current implementation)
        // But in a real system, we might want to reject this. Let's check that it's added correctly.
        book.add_order(order2);

        // Cancel the first order
        bool result = book.cancel_order("UNIQUE");
        assert_with_message(result, "Expected successful cancellation");

        // Try cancelling again - this should cancel the second order with the same ID
        result = book.cancel_order("UNIQUE");
        assert_with_message(result, "Expected successful cancellation of second order with same ID");

        // Try cancelling a third time - this should fail as no more orders with that ID
        result = book.cancel_order("UNIQUE");
        assert_with_message(!result, "Expected cancellation to fail for non-existent order");
    });

    // Test 4: Multiple Symbol Handling in Matching Engine
    tests.add_test("Multiple Symbol Handling", [&]() {
        MatchingEngine engine;

        // Add order books for different symbols
        engine.add_order_book("AAPL");
        engine.add_order_book("MSFT");
        engine.add_order_book("GOOGL");

        // Place orders for different symbols
        auto trades1 = engine.place_limit_order("AAPL", "A1", OrderSide::Buy, 100, 150.0);
        auto trades2 = engine.place_limit_order("MSFT", "M1", OrderSide::Buy, 100, 250.0);
        auto trades3 = engine.place_limit_order("GOOGL", "G1", OrderSide::Buy, 100, 2500.0);

        // No trades should occur yet
        assert_with_message(trades1.empty(), "Expected no trades for AAPL");
        assert_with_message(trades2.empty(), "Expected no trades for MSFT");
        assert_with_message(trades3.empty(), "Expected no trades for GOOGL");

        // Place matching orders for each symbol
        trades1 = engine.place_limit_order("AAPL", "A2", OrderSide::Sell, 100, 150.0);
        trades2 = engine.place_limit_order("MSFT", "M2", OrderSide::Sell, 100, 250.0);
        trades3 = engine.place_limit_order("GOOGL", "G2", OrderSide::Sell, 100, 2500.0);

        // Should have one trade per symbol
        assert_with_message(trades1.size() == 1, "Expected 1 trade for AAPL");
        assert_with_message(trades2.size() == 1, "Expected 1 trade for MSFT");
        assert_with_message(trades3.size() == 1, "Expected 1 trade for GOOGL");

        // Check trade details
        assert_with_message(trades1[0].order_id_buy == "A1", "Expected buyer A1");
        assert_with_message(trades2[0].order_id_buy == "M1", "Expected buyer M1");
        assert_with_message(trades3[0].order_id_buy == "G1", "Expected buyer G1");
    });

    // Test 5: Stress Test with Many Orders
    tests.add_test("Stress Test (Many Orders)", [&]() {
        OrderBook book("TEST");
        const int NUM_ORDERS = 1000;

        // Add many random buy and sell orders
        for (int i = 0; i < NUM_ORDERS; ++i) {
            OrderSide side = i % 2 == 0 ? OrderSide::Buy : OrderSide::Sell;
            auto order = generator.random_limit_order(side, "TEST");
            book.add_order(order);
        }

        // Place some market orders and ensure they match appropriately
        for (int i = 0; i < 10; ++i) {
            OrderSide side = i % 2 == 0 ? OrderSide::Buy : OrderSide::Sell;
            auto order = generator.random_market_order(side, "TEST");
            auto trades = book.match_order(order);

            // Not all market orders will create trades, depending on the random order book state
            if (!trades.empty()) {
                if (side == OrderSide::Buy) {
                    // Buy orders should execute at ask prices or better
                    for (const auto& trade : trades) {
                        assert_with_message(trade.price <= book.best_ask() || book.best_ask() == std::numeric_limits<double>::max(),
                                          "Buy trade price should be <= best ask");
                    }
                } else {
                    // Sell orders should execute at bid prices or better
                    for (const auto& trade : trades) {
                        assert_with_message(trade.price >= book.best_bid() || book.best_bid() == 0.0,
                                          "Sell trade price should be >= best bid");
                    }
                }
            }
        }
    });

    // Test 6: Order Cancellation Edge Cases
    tests.add_test("Order Cancellation Edge Cases", [&]() {
        MatchingEngine engine;
        engine.add_order_book("TEST");

        // Place an order
        engine.place_limit_order("TEST", "ORDER1", OrderSide::Buy, 100, 10.0);

        // Cancel it
        bool result = engine.cancel_order("ORDER1");
        assert_with_message(result, "Expected successful cancellation");

        // Cancel again - should fail
        result = engine.cancel_order("ORDER1");
        assert_with_message(!result, "Expected second cancellation to fail");

        // Cancel a non-existent order
        result = engine.cancel_order("NONEXISTENT");
        assert_with_message(!result, "Expected cancellation of non-existent order to fail");

        // Add another order
        engine.place_limit_order("TEST", "ORDER2", OrderSide::Buy, 100, 10.0);

        // Add an order book for a different symbol
        engine.add_order_book("OTHER");

        // Place an order with the same ID but different symbol
        engine.place_limit_order("OTHER", "ORDER2", OrderSide::Buy, 100, 10.0);

        // Cancel the order - should cancel the first one
        result = engine.cancel_order("ORDER2");
        assert_with_message(result, "Expected successful cancellation");

        // Cancel again - should cancel the second one
        result = engine.cancel_order("ORDER2");
        assert_with_message(result, "Expected successful cancellation of second order");
    });

    // Test 7: Market Orders with Insufficient Liquidity
    tests.add_test("Market Orders with Insufficient Liquidity", [&]() {
        OrderBook book("TEST");

        // Add some buy orders
        auto buy1 = std::make_shared<Order>("BUY1", OrderSide::Buy, "TEST", 100, 10.0, get_timestamp());
        auto buy2 = std::make_shared<Order>("BUY2", OrderSide::Buy, "TEST", 100, 9.0, get_timestamp());
        book.add_order(buy1);
        book.add_order(buy2);

        // Place a market sell order larger than available liquidity
        auto sell = std::make_shared<Order>("SELL1", OrderSide::Sell, "TEST", 300, get_timestamp());
        auto trades = book.match_order(sell);

        // Should create trades for available liquidity only
        assert_with_message(trades.size() == 2, "Expected 2 trades");
        assert_with_message(trades[0].size == 100, "Expected first trade size 100");
        assert_with_message(trades[1].size == 100, "Expected second trade size 100");

        // Sell order should be partially filled
        assert_with_message(sell->filled_size == 200, "Expected sell order to be filled by 200");
        assert_with_message(sell->remaining_size() == 100, "Expected 100 remaining in sell order");
    });

    // Test 8: Price-Time Priority Complex Scenario
    tests.add_test("Price-Time Priority Complex Scenario", [&]() {
        OrderBook book("TEST");

        // Add some buy orders with the same price but different times
        auto buy1 = std::make_shared<Order>("BUY1", OrderSide::Buy, "TEST", 100, 10.0, get_timestamp());
        auto buy2 = std::make_shared<Order>("BUY2", OrderSide::Buy, "TEST", 100, 10.0, get_timestamp());
        auto buy3 = std::make_shared<Order>("BUY3", OrderSide::Buy, "TEST", 100, 11.0, get_timestamp());
        auto buy4 = std::make_shared<Order>("BUY4", OrderSide::Buy, "TEST", 100, 9.0, get_timestamp());

        book.add_order(buy1);
        book.add_order(buy2);
        book.add_order(buy3);
        book.add_order(buy4);

        // Place a matching sell order
        auto sell = std::make_shared<Order>("SELL1", OrderSide::Sell, "TEST", 250, 9.0, get_timestamp());
        auto trades = book.match_order(sell);

        // Should match with buy3 first (best price), then buy1 (earlier time), then buy2
        assert_with_message(trades.size() == 3, "Expected 3 trades");
        assert_with_message(trades[0].order_id_buy == "BUY3", "Expected first trade with BUY3 (best price)");
        assert_with_message(trades[1].order_id_buy == "BUY1", "Expected second trade with BUY1 (earlier time)");
        assert_with_message(trades[2].order_id_buy == "BUY2", "Expected third trade with BUY2");

        // Verify trade sizes
        assert_with_message(trades[0].size == 100, "Expected trade size 100");
        assert_with_message(trades[1].size == 100, "Expected trade size 100");
        assert_with_message(trades[2].size == 50, "Expected trade size 50");

        // Verify order state
        assert_with_message(buy1->is_filled(), "BUY1 should be completely filled");
        assert_with_message(buy2->filled_size == 50, "BUY2 should be partially filled");
        assert_with_message(buy3->is_filled(), "BUY3 should be completely filled");
        assert_with_message(buy4->filled_size == 0, "BUY4 should not be filled (price too low)");
    });

    // Run all tests
    tests.run_all();

    return 0;
}
