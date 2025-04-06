#include "order_book.hpp"
#include "matching_engine.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <iomanip>
#include <algorithm>
#include <functional>

using namespace trading;

// Simple benchmarking framework
class BenchmarkSuite {
public:
    using BenchmarkFunc = std::function<void()>;

    void add_benchmark(const std::string& name, BenchmarkFunc bench, int iterations = 1000) {
        benchmarks_.push_back({name, bench, iterations});
    }

    void run_all() {
        std::cout << "=== Running Benchmarks ===" << std::endl;
        std::cout << std::left << std::setw(40) << "Benchmark"
                  << std::setw(15) << "Iterations"
                  << std::setw(15) << "Time (μs)"
                  << std::setw(15) << "Avg (μs)" << std::endl;
        std::cout << std::string(85, '-') << std::endl;

        for (const auto& [name, bench, iterations] : benchmarks_) {
            // Warmup
            bench();

            // Timed run
            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < iterations; ++i) {
                bench();
            }
            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            double avg = static_cast<double>(duration) / iterations;

            std::cout << std::left << std::setw(40) << name
                      << std::setw(15) << iterations
                      << std::setw(15) << duration
                      << std::setw(15) << std::fixed << std::setprecision(2) << avg << std::endl;
        }
    }

private:
    std::vector<std::tuple<std::string, BenchmarkFunc, int>> benchmarks_;
};

// Random order generation
class OrderGenerator {
public:
    OrderGenerator(uint64_t seed = 0) : generator_(seed) {}

    std::shared_ptr<Order> generate_limit_order(OrderSide side, const std::string& symbol, uint64_t timestamp) {
        static uint64_t order_count = 0;
        std::string order_id = "ORD" + std::to_string(++order_count);

        uint64_t size = size_dist_(generator_);
        double price = 0.0;

        if (side == OrderSide::Buy) {
            price = 100.0 + (buy_price_dist_(generator_) / 10.0);
        } else {
            price = 100.0 + (sell_price_dist_(generator_) / 10.0);
        }

        return std::make_shared<Order>(order_id, side, symbol, size, price, timestamp);
    }

    std::shared_ptr<Order> generate_market_order(OrderSide side, const std::string& symbol, uint64_t timestamp) {
        static uint64_t order_count = 0;
        std::string order_id = "MKT" + std::to_string(++order_count);
        uint64_t size = size_dist_(generator_);

        return std::make_shared<Order>(order_id, side, symbol, size, timestamp);
    }

private:
    std::mt19937_64 generator_;
    std::uniform_int_distribution<uint64_t> size_dist_{10, 1000};
    std::uniform_int_distribution<int> buy_price_dist_{950, 1050};  // 95.0 - 105.0
    std::uniform_int_distribution<int> sell_price_dist_{950, 1050}; // 95.0 - 105.0
};

// Get current timestamp in nanoseconds
uint64_t get_timestamp() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

int main() {
    BenchmarkSuite benchmarks;
    OrderGenerator generator(42); // Fixed seed for reproducibility

    // Benchmark 1: Order book creation
    benchmarks.add_benchmark("Order Book Creation", []() {
        OrderBook book("TEST");
    }, 10000);

    // Benchmark 2: Adding limit orders
    benchmarks.add_benchmark("Adding Limit Orders", [&]() {
        OrderBook book("TEST");
        uint64_t timestamp = get_timestamp();

        for (int i = 0; i < 100; ++i) {
            auto order = generator.generate_limit_order(
                i % 2 == 0 ? OrderSide::Buy : OrderSide::Sell,
                "TEST", timestamp + i
            );
            book.add_order(order);
        }
    }, 100);

    // Benchmark 3: Order matching
    benchmarks.add_benchmark("Order Matching", [&]() {
        OrderBook book("TEST");
        uint64_t timestamp = get_timestamp();

        // Add 100 buy orders
        for (int i = 0; i < 100; ++i) {
            auto order = generator.generate_limit_order(OrderSide::Buy, "TEST", timestamp + i);
            book.add_order(order);
        }

        // Add 100 sell orders and measure matching
        std::vector<Trade> all_trades;
        for (int i = 0; i < 100; ++i) {
            auto order = generator.generate_limit_order(OrderSide::Sell, "TEST", timestamp + 100 + i);
            auto trades = book.match_order(order);
            all_trades.insert(all_trades.end(), trades.begin(), trades.end());
            if (!trades.empty()) {
                book.add_order(order);
            }
        }
    }, 10);

    // Benchmark 4: Matching engine with multiple order books
    benchmarks.add_benchmark("Matching Engine Multiple Books", [&]() {
        MatchingEngine engine;
        const std::vector<std::string> symbols = {"AAPL", "MSFT", "GOOGL", "AMZN", "FB"};

        // Create order books
        for (const auto& symbol : symbols) {
            engine.add_order_book(symbol);
        }

        for (int i = 0; i < 100; ++i) {
            const std::string& symbol = symbols[i % symbols.size()];
            OrderSide side = i % 2 == 0 ? OrderSide::Buy : OrderSide::Sell;

            engine.place_limit_order(
                symbol,
                "ORD" + std::to_string(i),
                side,
                100 + (i % 900),
                100.0 + (i % 10)
            );
        }
    }, 10);

    // Benchmark 5: Cancel orders
    benchmarks.add_benchmark("Order Cancellation", [&]() {
        OrderBook book("TEST");
        std::vector<std::string> order_ids;
        uint64_t timestamp = get_timestamp();

        // Add orders
        for (int i = 0; i < 100; ++i) {
            auto order = generator.generate_limit_order(
                i % 2 == 0 ? OrderSide::Buy : OrderSide::Sell,
                "TEST", timestamp + i
            );
            order_ids.push_back(order->order_id);
            book.add_order(order);
        }

        // Cancel orders
        for (const auto& id : order_ids) {
            book.cancel_order(id);
        }
    }, 100);

    // Benchmark 6: Market orders
    benchmarks.add_benchmark("Market Order Execution", [&]() {
        OrderBook book("TEST");
        uint64_t timestamp = get_timestamp();

        // Add limit orders to provide liquidity
        for (int i = 0; i < 100; ++i) {
            auto order = generator.generate_limit_order(
                i % 2 == 0 ? OrderSide::Buy : OrderSide::Sell,
                "TEST", timestamp + i
            );
            book.add_order(order);
        }

        // Execute market orders
        for (int i = 0; i < 20; ++i) {
            auto order = generator.generate_market_order(
                i % 2 == 0 ? OrderSide::Buy : OrderSide::Sell,
                "TEST", timestamp + 100 + i
            );
            book.match_order(order);
        }
    }, 50);

    // Run all benchmarks
    benchmarks.run_all();

    return 0;
}
