# High-Performance C++20 Trading Engine

A high-performance matching engine for financial trading systems built with modern C++. This engine implements a sophisticated price-time priority algorithm to efficiently process and match buy and sell orders in financial markets.

## Key Features

- **Efficient Matching Algorithm**: Price-time priority based matching for optimal market fairness
- **Multi-Symbol Support**: Simultaneously manage order books for different financial instruments
- **Order Type Support**:
  - Limit orders (specify price and quantity)
  - Market orders (execute at best available price)
- **Comprehensive Testing**: Regular, advanced, and stress tests ensure system reliability
- **Performance Benchmarking**: Built-in benchmarks to measure and optimize system performance
- **Thread Safety**: Core components designed with thread-safety in mind for concurrent access

## System Architecture

The system consists of these primary components:

1. **OrderBook**: Maintains buy and sell orders for a single financial instrument.
   - Implements efficient lookup and matching algorithms
   - Supports duplicate order IDs with unordered_multimap
   - Manages price-time priority sorting

2. **MatchingEngine**: Manages multiple order books and provides the primary API.
   - Creates and manages order books for different symbols
   - Routes orders to appropriate order books
   - Provides callbacks for trade notifications

3. **Order Types**:
   - Limit orders with specified price boundaries
   - Market orders that execute at the best available price

## Performance

The engine is optimized for both speed and reliability:

- Fast order placement and cancellation
- Efficient matching algorithm with optimal time complexity
- Benchmarks show sub-millisecond performance for most operations

## Getting Started

### Prerequisites

- CMake 3.12 or higher
- C++20 compatible compiler (GCC 10+, Clang 10+, or MSVC 19.26+)

### Building the Project

```bash
mkdir -p build
cd build
cmake ..
make
```

### Running the Trading Engine

```bash
./trading_engine
```

### Running Tests

Regular tests:
```bash
./trading_tests
```

Advanced tests:
```bash
./advanced_tests
```

### Running Benchmarks

```bash
./trading_benchmarks
```

## Code Example

```cpp
// Create a matching engine
trading::MatchingEngine engine;

// Add order books for different symbols
engine.add_order_book("AAPL");
engine.add_order_book("MSFT");

// Place limit orders
auto trades1 = engine.place_limit_order("AAPL", "ORD1", trading::OrderSide::Buy, 100, 150.0);
auto trades2 = engine.place_limit_order("AAPL", "ORD2", trading::OrderSide::Sell, 100, 150.0);

// Check for matches
if (!trades2.empty()) {
    std::cout << "Trade executed: " << trades2[0].size << " shares at $"
              << trades2[0].price << std::endl;
}

// Place a market order
auto marketTrades = engine.place_market_order("MSFT", "ORD3", trading::OrderSide::Buy, 100);

// Cancel an order
bool success = engine.cancel_order("ORD1");
```

## Implementation Details

- **Memory Management**: Leverages modern C++ smart pointers for automatic resource management
- **Data Structures**: Utilizes STL containers with custom comparators for order priority
- **Error Handling**: Comprehensive validation for order parameters and state changes
- **Timestamp Precision**: Nanosecond precision for accurate time-based priority

## Performance Optimization

The engine is designed with performance in mind, including:

- Minimal memory allocations in the critical path
- Efficient lookup with O(1) time complexity for order retrieval
- Optimized sorting algorithms for price-time priority maintenance
- Lock-free implementations where possible

## License

[MIT License](LICENSE)
