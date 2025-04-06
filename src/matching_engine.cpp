#include "matching_engine.hpp"
#include <chrono>

namespace trading {

MatchingEngine::MatchingEngine() {
    // Nothing to initialize
}

void MatchingEngine::add_order_book(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if order book already exists
    if (order_books_.find(symbol) != order_books_.end()) {
        return; // Already exists, do nothing
    }

    // Create a new order book
    order_books_[symbol] = std::make_shared<OrderBook>(symbol);
}

std::vector<Trade> MatchingEngine::place_limit_order(
    const std::string& symbol,
    const std::string& order_id,
    OrderSide side,
    uint64_t size,
    double price) {

    std::lock_guard<std::mutex> lock(mutex_);

    // Find the order book
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) {
        return {}; // No such symbol
    }

    // Create the order
    auto order = std::make_shared<Order>(
        order_id, side, symbol, size, price, generate_timestamp());

    // Store the order ID to symbol mapping
    order_id_to_symbol_.insert({order_id, symbol});

    // Match the order
    auto trades = it->second->match_order(order);

    // If not fully filled, add to book
    if (!order->is_filled()) {
        it->second->add_order(order);
    }

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

    // Find the order book
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) {
        return {}; // No such symbol
    }

    // Create the order
    auto order = std::make_shared<Order>(
        order_id, side, symbol, size, generate_timestamp());

    // No need to store in symbol map as market orders don't rest in the book

    // Match the order
    auto trades = it->second->match_order(order);

    // Notify about trades
    for (const auto& trade : trades) {
        notify_trade(trade);
    }

    return trades;
}

bool MatchingEngine::cancel_order(const std::string& order_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Find the symbol for this order
    auto range = order_id_to_symbol_.equal_range(order_id);
    if (range.first == range.second) {
        return false; // Order ID not found
    }

    // Get the first matching order
    auto symbol_it = range.first;
    const std::string& symbol = symbol_it->second;

    // Find the order book
    auto book_it = order_books_.find(symbol);
    if (book_it == order_books_.end()) {
        return false; // Order book not found
    }

    // Cancel the order
    bool success = book_it->second->cancel_order(order_id);

    // If successful, remove only this specific entry from the multimap
    if (success) {
        order_id_to_symbol_.erase(symbol_it);
    }

    return success;
}

std::vector<std::shared_ptr<OrderBook>> MatchingEngine::get_all_order_books() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::shared_ptr<OrderBook>> result;
    result.reserve(order_books_.size());

    for (const auto& [symbol, book] : order_books_) {
        result.push_back(book);
    }

    return result;
}

std::shared_ptr<OrderBook> MatchingEngine::get_order_book(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) {
        return nullptr;
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
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

void MatchingEngine::notify_trade(const Trade& trade) {
    for (const auto& callback : trade_callbacks_) {
        callback(trade);
    }
}

} // namespace trading
