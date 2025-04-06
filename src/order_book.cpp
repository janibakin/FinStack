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
    // Note: This allows duplicate order_ids in map
    order_map_.insert({order->order_id, order});

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
    // Find the first order with this ID
    auto range = order_map_.equal_range(order_id);
    if (range.first == range.second) {
        return false; // Order not found
    }

    // Get the first matching order and mark it as cancelled
    auto it = range.first;
    auto order = it->second;
    order->status = OrderStatus::Cancelled;

    // Remove from vectors
    if (order->side == OrderSide::Buy) {
        // Find and remove this specific order instance
        auto order_it = std::find_if(buy_orders_.begin(), buy_orders_.end(),
            [&order](const auto& o) { return o.get() == order.get(); });

        if (order_it != buy_orders_.end()) {
            buy_orders_.erase(order_it);
        }
    } else {
        // Find and remove this specific order instance
        auto order_it = std::find_if(sell_orders_.begin(), sell_orders_.end(),
            [&order](const auto& o) { return o.get() == order.get(); });

        if (order_it != sell_orders_.end()) {
            sell_orders_.erase(order_it);
        }
    }

    // Remove from map - only remove this specific instance
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

            // If resting order is now filled, remove it
            if (match_order->is_filled()) {
                opposite_side.erase(opposite_side.begin());
                // Find all entries in the multimap with this ID and remove the one with the same pointer
                auto range = order_map_.equal_range(match_order->order_id);
                for (auto it = range.first; it != range.second; ++it) {
                    if (it->second.get() == match_order.get()) {
                        order_map_.erase(it);
                        break;
                    }
                }
            }
        } else {
            break; // No more price matches possible
        }
    }

    // Update last update time
    last_update_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    return trades;
}

void OrderBook::remove_filled_orders() {
    // Remove filled orders from both sides
    buy_orders_.erase(
        std::remove_if(buy_orders_.begin(), buy_orders_.end(),
            [](const auto& o) { return o->is_filled() || o->status == OrderStatus::Cancelled; }),
        buy_orders_.end());

    sell_orders_.erase(
        std::remove_if(sell_orders_.begin(), sell_orders_.end(),
            [](const auto& o) { return o->is_filled() || o->status == OrderStatus::Cancelled; }),
        sell_orders_.end());
}

void OrderBook::sort_buy_orders() {
    // Sort by price DESC, then by timestamp ASC (price-time priority)
    std::sort(buy_orders_.begin(), buy_orders_.end(),
              [](const auto& a, const auto& b) {
                  if (a->price != b->price) {
                      return a->price > b->price; // Higher buy prices first
                  }
                  return a->timestamp < b->timestamp; // Earlier timestamps first
              });
}

void OrderBook::sort_sell_orders() {
    // Sort by price ASC, then by timestamp ASC (price-time priority)
    std::sort(sell_orders_.begin(), sell_orders_.end(),
              [](const auto& a, const auto& b) {
                  if (a->price != b->price) {
                      return a->price < b->price; // Lower sell prices first
                  }
                  return a->timestamp < b->timestamp; // Earlier timestamps first
              });
}

double OrderBook::best_bid() const {
    if (buy_orders_.empty()) {
        return 0.0; // No bid
    }
    return buy_orders_.front()->price;
}

double OrderBook::best_ask() const {
    if (sell_orders_.empty()) {
        return std::numeric_limits<double>::max(); // No ask
    }
    return sell_orders_.front()->price;
}

uint64_t OrderBook::volume_at_price(OrderSide side, double price) const {
    uint64_t volume = 0;

    const auto& orders = (side == OrderSide::Buy) ? buy_orders_ : sell_orders_;

    for (const auto& order : orders) {
        if (order->price == price) {
            volume += order->remaining_size();
        }
    }

    return volume;
}

std::pair<std::vector<std::shared_ptr<Order>>, std::vector<std::shared_ptr<Order>>>
OrderBook::get_all_orders() const {
    return {buy_orders_, sell_orders_};
}

void OrderBook::print() const {
    std::cout << "Order Book: " << symbol_ << std::endl;

    // Print sells (highest to lowest)
    std::cout << "SELLS:" << std::endl;
    if (sell_orders_.empty()) {
        std::cout << "  [Empty]" << std::endl;
    } else {
        auto sells = sell_orders_;
        std::sort(sells.begin(), sells.end(),
                 [](const auto& a, const auto& b) { return a->price > b->price; });

        for (const auto& order : sells) {
            std::cout << "  " << order->price << " x " << order->remaining_size()
                      << " (" << order->order_id << ")" << std::endl;
        }
    }

    // Print buys (highest to lowest)
    std::cout << "BUYS:" << std::endl;
    if (buy_orders_.empty()) {
        std::cout << "  [Empty]" << std::endl;
    } else {
        for (const auto& order : buy_orders_) {
            std::cout << "  " << order->price << " x " << order->remaining_size()
                      << " (" << order->order_id << ")" << std::endl;
        }
    }
}

} // namespace trading
