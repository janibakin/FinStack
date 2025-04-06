#pragma once

#include "order_book.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mutex>

namespace trading {

// Callback type for trade notifications
using TradeCallback = std::function<void(const Trade&)>;

// Class that manages multiple order books and matches orders
class MatchingEngine {
public:
    MatchingEngine();

    // Add a new order book for a symbol
    void add_order_book(const std::string& symbol);

    // Place a limit order
    std::vector<Trade> place_limit_order(
        const std::string& symbol,
        const std::string& order_id,
        OrderSide side,
        uint64_t size,
        double price);

    // Place a market order
    std::vector<Trade> place_market_order(
        const std::string& symbol,
        const std::string& order_id,
        OrderSide side,
        uint64_t size);

    // Cancel an existing order
    bool cancel_order(const std::string& order_id);

    // Get all order books
    std::vector<std::shared_ptr<OrderBook>> get_all_order_books() const;

    // Get a specific order book
    std::shared_ptr<OrderBook> get_order_book(const std::string& symbol) const;

    // Register a callback to be notified of trades
    void register_trade_callback(TradeCallback callback);

    // Print the state of all order books
    void print_all() const;

private:
    std::unordered_map<std::string, std::shared_ptr<OrderBook>> order_books_;
    std::unordered_multimap<std::string, std::string> order_id_to_symbol_; // Allow multiple entries for same order ID
    std::vector<TradeCallback> trade_callbacks_;
    mutable std::mutex mutex_; // To protect concurrent access

    // Helper to generate a timestamp
    uint64_t generate_timestamp() const;

    // Notify all callbacks about a new trade
    void notify_trade(const Trade& trade);
};

} // namespace trading