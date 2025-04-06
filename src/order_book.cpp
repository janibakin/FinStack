#include "order_book.hpp"
#include <algorithm>
#include <iostream>
#include <chrono>

namespace trading {

OrderBook::OrderBook(std::string symbol)
    : symbol_(std::move(symbol)),
      last_update_time_(0) {
}

void OrderBook::add_order(std::shared_ptr<Order> order) {
    // Store order in the map for quick access by ID
    order_map_[order->order_id] = order;
    
    // Add to appropriate vector based on side
    if (order->side == OrderSide::Buy) {
        buy_orders_.push_back(order);
        sort_buy_orders();
    } else {
        sell_orders_.push_back(order);
        sort_sell_orders();
    }
    
    // Update last update time
    last_update_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool OrderBook::cancel_order(const std::string& order_id) {
    auto it = order_map_.find(order_id);
    if (it == order_map_.end()) {
        return false; // Order not found
    }
    
    auto order = it->second;
    order->status = OrderStatus::Cancelled;
    
    // Remove from vectors (will be cleaned up later)
    if (order->side == OrderSide::Buy) {
        buy_orders_.erase(
            std::remove_if(buy_orders_.begin(), buy_orders_.end(),
                [&order_id](const auto& o) { return o->order_id == order_id; }),
            buy_orders_.end());
    } else {
        sell_orders_.erase(
            std::remove_if(sell_orders_.begin(), sell_orders_.end(),
                [&order_id](const auto& o) { return o->order_id == order_id; }),
            sell_orders_.end());
    }
    
    // Remove from map
    order_map_.erase(it);
    
    // Update last update time
    last_update_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return true;
}

std::vector<Trade> OrderBook::match_order(std::shared_ptr<Order> order) {
    std::vector<Trade> trades;
    
    // Check which side we're matching against
    auto& opposite_side = (order->side == OrderSide::Buy) ? sell_orders_ : buy_orders_;
    
    // Process until order is filled or no more matches
    while (!opposite_side.empty() && !order->is_filled()) {
        auto match_order = opposite_side.front();
        
        // For buy orders, we match if the buy price >= sell price
        // For sell orders, we match if the sell price <= buy price
        bool price_matches = (order->side == OrderSide::Buy) 
                            ? order->price >= match_order->price
                            : order->price <= match_order->price;
        
        // If market order or price is acceptable
        if (order->type == OrderType::Market || price_matches) {
            // Calculate fill size
            uint64_t fill_size = std::min(order->remaining_size(), match_order->remaining_size());
            
            // Use the resting order's price
            double trade_price = match_order->price;
            
            // Update both orders
            order->fill(fill_size);
            match_order->fill(fill_size);
            
            // Create trade record
            uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            // Create trade with proper buyer/seller IDs
            if (order->side == OrderSide::Buy) {
                trades.emplace_back(order->order_id, match_order->order_id, 
                                   fill_size, trade_price, timestamp);
            } else {
                trades.emplace_back(match_order->order_id, order->order_id, 
                                   fill_size, trade_price, timestamp);
            }
            
            // If the matching order is completely filled, remove it from the book
            if (match_order->is_filled()) {
                opposite_side.erase(opposite_side.begin());
            }
        } else {
            // No more matches at acceptable prices
            break;
        }
    }
    
    // If the new order has remaining quantity, add it to the book
    if (!order->is_filled() && order->type == OrderType::Limit) {
        add_order(order);
    }
    
    // Clean up filled orders
    remove_filled_orders();
    
    return trades;
}

double OrderBook::best_bid() const {
    if (buy_orders_.empty()) {
        return 0.0; // No bids
    }
    return buy_orders_.front()->price;
}

double OrderBook::best_ask() const {
    if (sell_orders_.empty()) {
        return std::numeric_limits<double>::max(); // No asks
    }
    return sell_orders_.front()->price;
}

std::vector<std::shared_ptr<Order>> OrderBook::get_all_orders() const {
    std::vector<std::shared_ptr<Order>> all_orders;
    all_orders.reserve(buy_orders_.size() + sell_orders_.size());
    
    // Copy buy and sell orders
    all_orders.insert(all_orders.end(), buy_orders_.begin(), buy_orders_.end());
    all_orders.insert(all_orders.end(), sell_orders_.begin(), sell_orders_.end());
    
    return all_orders;
}

void OrderBook::print() const {
    std::cout << "Order Book for " << symbol_ << std::endl;
    
    // Print sell orders from highest to lowest
    std::cout << "Sell Orders:" << std::endl;
    for (auto it = sell_orders_.rbegin(); it != sell_orders_.rend(); ++it) {
        std::cout << "  Price: " << (*it)->price 
                  << ", Size: " << (*it)->remaining_size() 
                  << ", ID: " << (*it)->order_id << std::endl;
    }
    
    // Print the spread
    if (!buy_orders_.empty() && !sell_orders_.empty()) {
        double spread = best_ask() - best_bid();
        std::cout << "Spread: " << spread << std::endl;
    } else {
        std::cout << "No spread (incomplete book)" << std::endl;
    }
    
    // Print buy orders from highest to lowest
    std::cout << "Buy Orders:" << std::endl;
    for (const auto& order : buy_orders_) {
        std::cout << "  Price: " << order->price 
                  << ", Size: " << order->remaining_size() 
                  << ", ID: " << order->order_id << std::endl;
    }
}

void OrderBook::sort_buy_orders() {
    // Sort by price (highest first) then time (earliest first)
    std::sort(buy_orders_.begin(), buy_orders_.end(), 
              [](const auto& lhs, const auto& rhs) {
                  if (lhs->price == rhs->price) {
                      return lhs->timestamp < rhs->timestamp;
                  }
                  return lhs->price > rhs->price;
              });
}

void OrderBook::sort_sell_orders() {
    // Sort by price (lowest first) then time (earliest first)
    std::sort(sell_orders_.begin(), sell_orders_.end(), 
              [](const auto& lhs, const auto& rhs) {
                  if (lhs->price == rhs->price) {
                      return lhs->timestamp < rhs->timestamp;
                  }
                  return lhs->price < rhs->price;
              });
}

void OrderBook::remove_filled_orders() {
    // Remove filled orders from buy side
    buy_orders_.erase(
        std::remove_if(buy_orders_.begin(), buy_orders_.end(),
            [](const auto& order) { return order->is_filled(); }),
        buy_orders_.end());
    
    // Remove filled orders from sell side
    sell_orders_.erase(
        std::remove_if(sell_orders_.begin(), sell_orders_.end(),
            [](const auto& order) { return order->is_filled(); }),
        sell_orders_.end());
}

} // namespace trading