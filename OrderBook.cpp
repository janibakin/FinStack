#include <iostream>
#include <vector>
#include <string>
#include <stack>
#include <algorithm>
#include <cstdint>
#include <format>
#include <stdexcept>


enum class OrderType {
    GoodTillCancel,
    FillAndKill
};

enum class Side {
    Buy,
    Sell
};

using Price = int32_t;
using Quantity = uint64_t;
using OrderId = uint64_t;


struct LevelInfo {
    Price _price;
    Quantity _quantity;
};

using LevelInfos = std::vector<LevelInfo>;

class OrderBookLevelInfos {
public:
    OrderBookLevelInfos(const LevelInfos& bids, const LevelInfos& asks)
        : _bids(bids), _asks(asks)
    {};

    const LevelInfos& getBids() const { return _bids; }
    const LevelInfos& getAsks() const { return _asks; }
     
private:
    LevelInfos _bids;
    LevelInfos _asks;
};

class Order {
public:
    Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
        : _type { orderType },
        orderId { orderId },
        _side { side },
        _price { price },
        _initialQuantity { quantity },
        _remainingQuantity { quantity }
    {};

    OrderId getOrderId() const { return orderId; }
    Side getSide() const { return _side; }
    Price getPrice() const { return _price; }
    OrderType getOrderPrice() const { return _type; }
    Quantity getInitialQuantity() const { return _initialQuantity; }
    Quantity getRemainingQuantity() const { return _remainingQuantity; }
    Quantity getFilledQuantity() const { return getInitialQuantity() - getRemainingQuantity(); }
    
    void Fill(Quantity quantity) { 
        if(quantity > getRemainingQuantity()) {
            // throw std::logic_error(std::format("Order ({}) cannot fill more than remaining quantity"), getOrderId());
            throw std::logic_error("Order: (" + std::to_string(getOrderId()) + ") cannot fill more than remaining quantity. ");
        }
        _remainingQuantity -= quantity;
    }

private:
    OrderType _type;
    OrderId orderId;
    Side _side;
    Price _price;
    Quantity _initialQuantity;
    Quantity _remainingQuantity;
};

int main() {
    LevelInfos bids = { {100, 10}, {99, 20}, {98, 30} };
    LevelInfos asks = { {101, 10}, {102, 20}, {103, 30} };
    Order order(OrderType::GoodTillCancel, 1, Side::Buy, 99, 10);
    OrderBookLevelInfos orderBookLevelInfos(bids, asks);
    std::cout << "Order Id: " << order.getOrderId() << std::endl;
    std::cout << "Order Side: " << (order.getSide() == Side::Buy ? "Buy" : "Sell") << std::endl;
    std::cout << "Order Price: " << order.getPrice() << std::endl;
    std::cout << "Order Initial Quantity: " << order.getInitialQuantity() << std::endl;
    std::cout << "Order Remaining Quantity: " << order.getRemainingQuantity() << std::endl;
    std::cout << "Order Filled Quantity: " << order.getFilledQuantity() << std::endl;
    order.Fill(5);
    std::cout << "Order Remaining Quantity: " << order.getRemainingQuantity() << std::endl;
    order.Fill(5);
    std::cout << "Order Remaining Quantity: " << order.getRemainingQuantity() << std::endl;
    try {
        order.Fill(1);
    } catch(const std::logic_error& e) {
        std::cout << e.what() << std::endl;
    }
    return EXIT_SUCCESS;
}