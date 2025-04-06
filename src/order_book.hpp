#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <limits>
#include <algorithm>
#include <iostream>
#include <atomic>

namespace trading {

// Enum representing the side of an order (buy or sell)
enum class OrderSide : uint8_t {
    Buy,
    Sell
};

// Enum representing the type of an order
enum class OrderType : uint8_t {
    Limit,  // Order with a specific price
    Market  // Order at best available price
};

// Order status
enum class OrderStatus : uint8_t {
    New,        // Just created
    PartiallyFilled, // Partially executed
    Filled,     // Fully executed
    Cancelled,  // Cancelled by user
    Rejected    // Rejected by system
};

// Simple structure representing a trade
struct Trade {
    std::string order_id_buy;
    std::string order_id_sell;
    uint64_t size;
    double price;
    uint64_t timestamp;

    // Constructor
    Trade(const std::string& buy_id, const std::string& sell_id,
          uint64_t qty, double prc, uint64_t time)
        : order_id_buy(buy_id), order_id_sell(sell_id),
          size(qty), price(prc), timestamp(time) {}
};

// Structure representing an order in the system
struct Order {
    std::string order_id;    // Unique identifier
    OrderSide side;          // Buy or Sell
    OrderType type;          // Limit or Market
    std::string symbol;      // Trading symbol/instrument
    uint64_t size;           // Original order size
    uint64_t filled_size;    // Amount that has been filled
    double price;            // Limit price (for limit orders)
    uint64_t timestamp;      // When the order was placed
    OrderStatus status;      // Current status
    
    // Constructor for a limit order
    Order(std::string id, OrderSide s, std::string sym, 
          uint64_t sz, double prc, uint64_t time)
        : order_id(std::move(id)), side(s), type(OrderType::Limit),
          symbol(std::move(sym)), size(sz), filled_size(0),
          price(prc), timestamp(time), status(OrderStatus::New) {}
    
    // Constructor for a market order
    Order(std::string id, OrderSide s, std::string sym, 
          uint64_t sz, uint64_t time)
        : order_id(std::move(id)), side(s), type(OrderType::Market),
          symbol(std::move(sym)), size(sz), filled_size(0),
          price(s == OrderSide::Buy ? std::numeric_limits<double>::max() 
                                  : 0.0),
          timestamp(time), status(OrderStatus::New) {}
    
    // Remaining quantity
    uint64_t remaining_size() const { 
        return size - filled_size; 
    }
    
    // Check if order is completely filled
    bool is_filled() const { 
        return filled_size >= size; 
    }
    
    // Update order after a fill
    void fill(uint64_t fill_size) {
        filled_size += fill_size;
        if (is_filled()) {
            status = OrderStatus::Filled;
        } else {
            status = OrderStatus::PartiallyFilled;
        }
    }
};

// Comparison functor for buy orders (highest price first, then earliest time)
struct BuyOrderComparator {
    bool operator()(const std::shared_ptr<Order>& lhs, const std::shared_ptr<Order>& rhs) const {
        if (lhs->price == rhs->price) {
            return lhs->timestamp > rhs->timestamp; // Earlier timestamp first
        }
        return lhs->price < rhs->price; // Higher price first
    }
};

// Comparison functor for sell orders (lowest price first, then earliest time)
struct SellOrderComparator {
    bool operator()(const std::shared_ptr<Order>& lhs, const std::shared_ptr<Order>& rhs) const {
        if (lhs->price == rhs->price) {
            return lhs->timestamp > rhs->timestamp; // Earlier timestamp first
        }
        return lhs->price > rhs->price; // Lower price first
    }
};

// Class representing an order book for a single instrument
class OrderBook {
public:
    explicit OrderBook(std::string symbol);
    
    // Add a new order to the book
    void add_order(std::shared_ptr<Order> order);
    
    // Cancel an existing order
    bool cancel_order(const std::string& order_id);
    
    // Match an incoming order against the book
    std::vector<Trade> match_order(std::shared_ptr<Order> order);
    
    // Get the current best bid price
    double best_bid() const;
    
    // Get the current best ask price
    double best_ask() const;
    
    // Get the symbol this order book is for
    const std::string& get_symbol() const { return symbol_; }
    
    // Get all orders in the book
    std::vector<std::shared_ptr<Order>> get_all_orders() const;
    
    // Print the current state of the order book
    void print() const;

private:
    std::string symbol_;
    std::vector<std::shared_ptr<Order>> buy_orders_;  // Sorted by price-time priority
    std::vector<std::shared_ptr<Order>> sell_orders_; // Sorted by price-time priority
    std::unordered_map<std::string, std::shared_ptr<Order>> order_map_; // For fast lookup by ID
    std::atomic<uint64_t> last_update_time_;
    
    // Helper methods to keep the order vectors sorted
    void sort_buy_orders();
    void sort_sell_orders();
    
    // Remove filled orders from the book
    void remove_filled_orders();
};

} // namespace trading