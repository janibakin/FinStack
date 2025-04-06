#include "matching_engine.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace trading;

// Function to generate a unique order ID
std::string generate_order_id()
{
    static int counter = 0;
    std::stringstream ss;
    ss << "ORD" << std::setfill('0') << std::setw(6) << ++counter;
    return ss.str();
}

// Function to print trade information
void print_trade(const Trade &trade)
{
    std::cout << "TRADE: " << trade.order_id_buy << " bought "
              << trade.size << " @ $" << std::fixed << std::setprecision(2)
              << trade.price << " from " << trade.order_id_sell << std::endl;
}

int main()
{
    std::cout << "=== C++20 Trading Engine Demo ===" << std::endl;

    // Create a matching engine
    MatchingEngine engine;

    // Register a callback for trade notifications
    engine.register_trade_callback(print_trade);

    // Create order books for some symbols
    engine.add_order_book("AAPL");
    engine.add_order_book("MSFT");
    engine.add_order_book("GOOGL");

    std::cout << "Created order books for AAPL, MSFT, and GOOGL" << std::endl;

    // Place some limit orders
    std::cout << "\nPlacing initial orders..." << std::endl;

    // AAPL buy orders
    engine.place_limit_order("AAPL", generate_order_id(), OrderSide::Buy, 100, 150.0);
    engine.place_limit_order("AAPL", generate_order_id(), OrderSide::Buy, 200, 149.5);
    engine.place_limit_order("AAPL", generate_order_id(), OrderSide::Buy, 300, 149.0);

    // AAPL sell orders
    engine.place_limit_order("AAPL", generate_order_id(), OrderSide::Sell, 150, 150.5);
    engine.place_limit_order("AAPL", generate_order_id(), OrderSide::Sell, 250, 151.0);
    engine.place_limit_order("AAPL", generate_order_id(), OrderSide::Sell, 350, 151.5);

    // MSFT orders
    engine.place_limit_order("MSFT", generate_order_id(), OrderSide::Buy, 100, 250.0);
    engine.place_limit_order("MSFT", generate_order_id(), OrderSide::Sell, 100, 251.0);

    // Print the state of the order books
    std::cout << "\nInitial order book state:" << std::endl;
    engine.print_all();

    // Place a matching order that will execute
    std::cout << "\nPlacing a matching order (buy AAPL @ 151.0)..." << std::endl;
    auto buy_trades = engine.place_limit_order("AAPL", generate_order_id(), OrderSide::Buy, 200, 151.0);

    // Print the state of the order books after the trade
    std::cout << "\nOrder book state after buy order:" << std::endl;
    engine.print_all();

    // Place a market sell order
    std::cout << "\nPlacing a market sell order for AAPL..." << std::endl;
    auto sell_trades = engine.place_market_order("AAPL", generate_order_id(), OrderSide::Sell, 300);

    // Print the state of the order books after the market order
    std::cout << "\nOrder book state after market sell order:" << std::endl;
    engine.print_all();

    // Cancel an order
    std::string cancel_id = generate_order_id();
    std::cout << "\nPlacing an order to cancel: " << cancel_id << std::endl;
    engine.place_limit_order("MSFT", cancel_id, OrderSide::Buy, 50, 249.5);

    std::cout << "Order book state before cancellation:" << std::endl;
    engine.get_order_book("MSFT")->print();

    bool cancelled = engine.cancel_order(cancel_id);
    std::cout << "Cancel result: " << (cancelled ? "successful" : "failed") << std::endl;

    std::cout << "Order book state after cancellation:" << std::endl;
    engine.get_order_book("MSFT")->print();

    // Summary
    std::cout << "\n=== Trading Summary ===" << std::endl;
    std::cout << "Total buy trades executed: " << buy_trades.size() << std::endl;
    std::cout << "Total sell trades executed: " << sell_trades.size() << std::endl;

    return 0;
}