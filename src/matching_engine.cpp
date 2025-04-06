#include "matching_engine.hpp"
#include <iostream>
#include <chrono>

namespace trading {

MatchingEngine::MatchingEngine() = default;

void MatchingEngine::add_order_book(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (order_books_.find(symbol) == order_books_.end()) {
        order_books_[symbol] = std::make_shared<OrderBook>(symbol);
    }
}

std::vector<Trade> MatchingEngine::place_limit_order(
    const std::string& symbol, 
    const std::string& order_id,
    OrderSide side, 
    uint64_t size, 
    double price) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Make sure the order book exists
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) {
        std::cout << "Order book for symbol " << symbol << " not found" << std::endl;
        return {};
    }
    
    // Create the order
    auto timestamp = generate_timestamp();
    auto order = std::make_shared<Order>(order_id, side, symbol, size, price, timestamp);
    
    // Store mapping from order ID to symbol for cancellation
    order_id_to_symbol_[order_id] = symbol;
    
    // Match the order
    auto trades = it->second->match_order(order);
    
    // Notify about trades
    for (const auto& trade : trades) {
        notify_trade(trade);
    }
    
    return trades;
}

std::vector<Trade> MatchingEngine::place_market_order(
    const std::string& symbol, 
    const std::string& order_id,
    OrderSide side, 
    uint64_t size) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Make sure the order book exists
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) {
        std::cout << "Order book for symbol " << symbol << " not found" << std::endl;
        return {};
    }
    
    // Create the order
    auto timestamp = generate_timestamp();
    auto order = std::make_shared<Order>(order_id, side, symbol, size, timestamp);
    
    // Store mapping from order ID to symbol for cancellation (temporary for market orders)
    order_id_to_symbol_[order_id] = symbol;
    
    // Match the order
    auto trades = it->second->match_order(order);
    
    // Notify about trades
    for (const auto& trade : trades) {
        notify_trade(trade);
    }
    
    // Remove order ID mapping for market orders once processed
    order_id_to_symbol_.erase(order_id);
    
    return trades;
}

bool MatchingEngine::cancel_order(const std::string& order_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find the symbol for this order
    auto symbol_it = order_id_to_symbol_.find(order_id);
    if (symbol_it == order_id_to_symbol_.end()) {
        return false; // Order ID not found
    }
    
    // Find the order book
    auto book_it = order_books_.find(symbol_it->second);
    if (book_it == order_books_.end()) {
        return false; // Order book not found
    }
    
    // Cancel the order
    bool success = book_it->second->cancel_order(order_id);
    
    // If successful, remove from our mapping
    if (success) {
        order_id_to_symbol_.erase(symbol_it);
    }
    
    return success;
}

std::vector<std::shared_ptr<OrderBook>> MatchingEngine::get_all_order_books() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<OrderBook>> result;
    result.reserve(order_books_.size());
    
    for (const auto& [_, book] : order_books_) {
        result.push_back(book);
    }
    
    return result;
}

std::shared_ptr<OrderBook> MatchingEngine::get_order_book(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) {
        return nullptr; // Order book not found
    }
    
    return it->second;
}

void MatchingEngine::register_trade_callback(TradeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    trade_callbacks_.push_back(std::move(callback));
}

void MatchingEngine::print_all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::cout << "=== Matching Engine State ===" << std::endl;
    for (const auto& [symbol, book] : order_books_) {
        book->print();
        std::cout << "--------------------------" << std::endl;
    }
}

uint64_t MatchingEngine::generate_timestamp() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void MatchingEngine::notify_trade(const Trade& trade) {
    for (const auto& callback : trade_callbacks_) {
        callback(trade);
    }
}

} // namespace trading