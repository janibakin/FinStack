#include "order_book.hpp"
#include "matching_engine.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <functional>

using namespace trading;

// Simple test framework
class TestSuite {
public:
    using TestCase = std::function<void()>;
    
    void add_test(const std::string& name, TestCase test) {
        tests_.push_back({name, test});
    }
    
    void run_all() {
        std::cout << "Running " << tests_.size() << " tests..." << std::endl;
        
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
        
        std::cout << "\nResults: " << passed << "/" << tests_.size() << " tests passed" << std::endl;
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

int main() {
    TestSuite tests;
    
    // Test order book basics
    tests.add_test("OrderBook Creation", []() {
        OrderBook book("TEST");
        assert_with_message(book.get_symbol() == "TEST", "Symbol mismatch");
        assert_with_message(book.best_bid() == 0.0, "Expected no bids");
        assert_with_message(book.best_ask() == std::numeric_limits<double>::max(), "Expected no asks");
    });
    
    // Test limit order placement
    tests.add_test("Limit Order Placement", []() {
        OrderBook book("TEST");
        
        // Add a buy order
        auto buy_order = std::make_shared<Order>("BUY1", OrderSide::Buy, "TEST", 100, 10.0, 1);
        book.add_order(buy_order);
        
        assert_with_message(book.best_bid() == 10.0, "Buy order not reflected in best bid");
        
        // Add a sell order
        auto sell_order = std::make_shared<Order>("SELL1", OrderSide::Sell, "TEST", 100, 11.0, 2);
        book.add_order(sell_order);
        
        assert_with_message(book.best_ask() == 11.0, "Sell order not reflected in best ask");
    });
    
    // Test order matching
    tests.add_test("Order Matching", []() {
        OrderBook book("TEST");
        
        // Add a sell order
        auto sell_order = std::make_shared<Order>("SELL1", OrderSide::Sell, "TEST", 100, 10.0, 1);
        book.add_order(sell_order);
        
        // Add a matching buy order
        auto buy_order = std::make_shared<Order>("BUY1", OrderSide::Buy, "TEST", 100, 10.0, 2);
        auto trades = book.match_order(buy_order);
        
        assert_with_message(trades.size() == 1, "Expected 1 trade");
        assert_with_message(trades[0].size == 100, "Expected trade size 100");
        assert_with_message(trades[0].price == 10.0, "Expected trade price 10.0");
    });
    
    // Test order cancellation
    tests.add_test("Order Cancellation", []() {
        OrderBook book("TEST");
        
        // Add an order
        auto order = std::make_shared<Order>("ORDER1", OrderSide::Buy, "TEST", 100, 10.0, 1);
        book.add_order(order);
        
        // Cancel it
        bool result = book.cancel_order("ORDER1");
        assert_with_message(result, "Order cancellation failed");
        
        // Try to cancel non-existent order
        result = book.cancel_order("NONEXISTENT");
        assert_with_message(!result, "Cancelling non-existent order should fail");
    });
    
    // Test matching engine
    tests.add_test("Matching Engine", []() {
        MatchingEngine engine;
        
        // Add an order book
        engine.add_order_book("TEST");
        
        // Place some orders
        engine.place_limit_order("TEST", "SELL1", OrderSide::Sell, 100, 10.0);
        auto trades = engine.place_limit_order("TEST", "BUY1", OrderSide::Buy, 100, 10.0);
        
        assert_with_message(trades.size() == 1, "Expected 1 trade");
        assert_with_message(trades[0].size == 100, "Expected trade size 100");
    });
    
    // Test price-time priority
    tests.add_test("Price-Time Priority", []() {
        OrderBook book("TEST");
        
        // Add sell orders
        auto sell1 = std::make_shared<Order>("SELL1", OrderSide::Sell, "TEST", 100, 10.0, 1);
        auto sell2 = std::make_shared<Order>("SELL2", OrderSide::Sell, "TEST", 100, 10.0, 2);
        auto sell3 = std::make_shared<Order>("SELL3", OrderSide::Sell, "TEST", 100, 9.0, 3);
        
        book.add_order(sell1);
        book.add_order(sell2);
        book.add_order(sell3);
        
        // Test price priority - lower sell price should be first
        assert_with_message(book.best_ask() == 9.0, "Expected best ask 9.0");
        
        // Match with a buy order
        auto buy = std::make_shared<Order>("BUY1", OrderSide::Buy, "TEST", 200, 10.0, 4);
        auto trades = book.match_order(buy);
        
        // Should match with sell3 first (price priority), then sell1 (time priority over sell2)
        assert_with_message(trades.size() == 2, "Expected 2 trades");
        assert_with_message(trades[0].order_id_sell == "SELL3", "Expected to match with SELL3 first");
        assert_with_message(trades[1].order_id_sell == "SELL1", "Expected to match with SELL1 second");
    });
    
    // Run all tests
    tests.run_all();
    
    return 0;
}