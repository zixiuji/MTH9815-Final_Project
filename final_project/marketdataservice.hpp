/**
 * marketdataservice.hpp
 * Defines the data types and Service for order book market data.
 *
 * @author
 *   Breman Thuraisingham
 * @coauthor
 *   Zixiuji Wang
 */

#ifndef MARKET_DATA_SERVICE_HPP
#define MARKET_DATA_SERVICE_HPP

#include "soa.hpp"
#include "utility.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <iostream>

using namespace std;

// ============================================================================
// ENUM: PricingSide
// ============================================================================
/**
 * Side for market data.
 */
enum PricingSide
{
    BID,
    OFFER
};

// ============================================================================
// CLASS: Order
// ============================================================================
/**
 * An order with price, quantity, and pricing side.
 */
class Order
{
public:
    // Constructors
    Order() = default;
    Order(double _price, long _quantity, PricingSide _side);

    // Accessors
    double GetPrice() const;
    long GetQuantity() const;
    PricingSide GetSide() const;

private:
    double price;
    long quantity;
    PricingSide side;
};

// ============================================================================
// CLASS: BidOffer
// ============================================================================
/**
 * A class representing a bid and offer order pair.
 */
class BidOffer
{
public:
    BidOffer() = default;
    BidOffer(const Order& _bidOrder, const Order& _offerOrder);

    const Order& GetBidOrder() const;
    const Order& GetOfferOrder() const;

private:
    Order bidOrder;
    Order offerOrder;
};

// ============================================================================
// CLASS: OrderBook<T>
// ============================================================================
/**
 * An order book containing a bid stack and an offer stack.
 * Type T is the product type.
 */
template <typename T>
class OrderBook
{
public:
    // Constructors
    OrderBook() = default;
    OrderBook(const T& _product, const vector<Order>& _bidStack,
              const vector<Order>& _offerStack);

    // Accessors
    const T& GetProduct() const;
    const vector<Order>& GetBidStack() const;
    const vector<Order>& GetOfferStack() const;

    // Retrieve the highest bid and the lowest offer
    const BidOffer GetBidOffer() const;

private:
    T product;
    vector<Order> bidStack;
    vector<Order> offerStack;
};

// Forward declaration
template <typename T>
class MarketDataConnector;

// ============================================================================
// CLASS: MarketDataService<T>
// ============================================================================
/**
 * MarketDataService that distributes market data keyed by product identifier.
 * Type T is the product type.
 */
template <typename T>
class MarketDataService : public Service<string, OrderBook<T>>
{
public:
    // Constructor / Destructor
    MarketDataService();
    ~MarketDataService();

    // Retrieve an OrderBook<T> by product ID
    OrderBook<T>& GetData(string _key) override;

    // Callback for new or updated data invoked by the connector
    void OnMessage(OrderBook<T>& _data) override;

    // Add a listener
    void AddListener(ServiceListener<OrderBook<T>>* listener) override;

    // Retrieve all listeners
    const vector<ServiceListener<OrderBook<T>>*>& GetListeners() const override;

    // Get the associated connector
    MarketDataConnector<T>* GetConnector();

    // Return the current depth for each order book
    int GetOrderBookDepth() const;

    // Return the best (highest bid, lowest offer) for a given product ID
    const BidOffer GetBestBidOffer(const string& productId);

    // Aggregate orders at each price level for a given product ID
    const OrderBook<T>& AggregateDepth(const string& productId);

private:
    // Internal container of order books, keyed by product identifier
    map<string, OrderBook<T>> orderBooks;

    // Listeners
    vector<ServiceListener<OrderBook<T>>*> listeners;

    // Connector
    MarketDataConnector<T>* connector;

    // Default book depth
    int bookDepth;
};

// ============================================================================
// CLASS: MarketDataConnector<T>
// ============================================================================
/**
 * Connector for market data subscription/publishing.
 * Type T is the product type.
 */
template <typename T>
class MarketDataConnector : public Connector<OrderBook<T>>
{
public:
    // Constructor
    MarketDataConnector(MarketDataService<T>* _service);

    // Publish data to the connector (not used here)
    void Publish(OrderBook<T>& _data) override;

    // Subscribe data from an ifstream
    void Subscribe(ifstream& data) override;

private:
    // Pointer to the MarketDataService
    MarketDataService<T>* service;
};

// ============================================================================
// IMPLEMENTATION: Order
// ============================================================================
Order::Order(double _price, long _quantity, PricingSide _side)
    : price(_price), quantity(_quantity), side(_side)
{
}

double Order::GetPrice() const
{
    return price;
}

long Order::GetQuantity() const
{
    return quantity;
}

PricingSide Order::GetSide() const
{
    return side;
}

// ============================================================================
// IMPLEMENTATION: BidOffer
// ============================================================================
BidOffer::BidOffer(const Order& _bidOrder, const Order& _offerOrder)
    : bidOrder(_bidOrder), offerOrder(_offerOrder)
{
}

const Order& BidOffer::GetBidOrder() const
{
    return bidOrder;
}

const Order& BidOffer::GetOfferOrder() const
{
    return offerOrder;
}

// ============================================================================
// IMPLEMENTATION: OrderBook<T>
// ============================================================================
template <typename T>
OrderBook<T>::OrderBook(const T& _product,
                        const vector<Order>& _bidStack,
                        const vector<Order>& _offerStack)
    : product(_product), bidStack(_bidStack), offerStack(_offerStack)
{
}

template <typename T>
const T& OrderBook<T>::GetProduct() const
{
    return product;
}

template <typename T>
const vector<Order>& OrderBook<T>::GetBidStack() const
{
    return bidStack;
}

template <typename T>
const vector<Order>& OrderBook<T>::GetOfferStack() const
{
    return offerStack;
}

template <typename T>
const BidOffer OrderBook<T>::GetBidOffer() const
{
    // Determine highest bid
    Order bestBid(bidStack[0]);
    double bestBidPrice = bestBid.GetPrice();
    for (size_t i = 1; i < bidStack.size(); ++i)
    {
        double currentPrice = bidStack[i].GetPrice();
        if (currentPrice > bestBidPrice)
        {
            bestBid = bidStack[i];
            bestBidPrice = currentPrice;
        }
    }

    // Determine lowest offer
    Order bestOffer(offerStack[0]);
    double bestOfferPrice = bestOffer.GetPrice();
    for (size_t j = 1; j < offerStack.size(); ++j)
    {
        double currentPrice = offerStack[j].GetPrice();
        if (currentPrice < bestOfferPrice)
        {
            bestOffer = offerStack[j];
            bestOfferPrice = currentPrice;
        }
    }
    return BidOffer(bestBid, bestOffer);
}

// ============================================================================
// IMPLEMENTATION: MarketDataService<T>
// ============================================================================
template <typename T>
MarketDataService<T>::MarketDataService()
    : orderBooks(), listeners(), connector(nullptr), bookDepth(10)
{
    connector = new MarketDataConnector<T>(this);
}

template <typename T>
MarketDataService<T>::~MarketDataService()
{
    delete connector;
}

template <typename T>
OrderBook<T>& MarketDataService<T>::GetData(string _key)
{
    return orderBooks[_key];
}

template <typename T>
void MarketDataService<T>::OnMessage(OrderBook<T>& _data)
{
    // Insert or update the order book
    string prodId = _data.GetProduct().GetProductId();
    orderBooks[prodId] = _data;

    // Notify all listeners
    for (auto& ls : listeners)
    {
        ls->ProcessAdd(_data);
    }
}

template <typename T>
void MarketDataService<T>::AddListener(ServiceListener<OrderBook<T>>* listener)
{
    listeners.push_back(listener);
}

template <typename T>
const vector<ServiceListener<OrderBook<T>>*>& MarketDataService<T>::GetListeners() const
{
    return listeners;
}

template <typename T>
MarketDataConnector<T>* MarketDataService<T>::GetConnector()
{
    return connector;
}

template <typename T>
int MarketDataService<T>::GetOrderBookDepth() const
{
    return bookDepth;
}

template <typename T>
const BidOffer MarketDataService<T>::GetBestBidOffer(const string& productId)
{
    return orderBooks[productId].GetBidOffer();
}

template <typename T>
const OrderBook<T>& MarketDataService<T>::AggregateDepth(const string& productId)
{
    // Retrieve original product and stacks
    T& productRef = orderBooks[productId].GetProduct();
    vector<Order>& originalBids = const_cast<vector<Order>&>(orderBooks[productId].GetBidStack());
    vector<Order>& originalOffers = const_cast<vector<Order>&>(orderBooks[productId].GetOfferStack());

    // Build aggregated bid map
    unordered_map<double, long> aggregatedBids;
    for (auto& oneBid : originalBids)
    {
        double p = oneBid.GetPrice();
        long q = oneBid.GetQuantity();
        aggregatedBids[p] += q;
    }

    // Create new bid stack
    vector<Order> newBidStack;
    for (auto& kv : aggregatedBids)
    {
        double aggPrice = kv.first;
        long aggQty = kv.second;
        Order bidOrder(aggPrice, aggQty, BID);
        newBidStack.push_back(bidOrder);
    }

    // Build aggregated offer map
    unordered_map<double, long> aggregatedOffers;
    for (auto& oneOffer : originalOffers)
    {
        double p = oneOffer.GetPrice();
        long q = oneOffer.GetQuantity();
        aggregatedOffers[p] += q;
    }

    // Create new offer stack
    vector<Order> newOfferStack;
    for (auto& kv : aggregatedOffers)
    {
        double aggPrice = kv.first;
        long aggQty = kv.second;
        Order offerOrder(aggPrice, aggQty, OFFER);
        newOfferStack.push_back(offerOrder);
    }

    // Create a temporary aggregated OrderBook
    // Note: This returns a temporary object. If you want to store it,
    // you could update orderBooks[productId] with it.
    static OrderBook<T> aggregatedBook(productRef, newBidStack, newOfferStack);
    aggregatedBook = OrderBook<T>(productRef, newBidStack, newOfferStack);
    return aggregatedBook;
}

// ============================================================================
// IMPLEMENTATION: MarketDataConnector<T>
// ============================================================================
template <typename T>
MarketDataConnector<T>::MarketDataConnector(MarketDataService<T>* _service)
    : service(_service)
{
}

template <typename T>
void MarketDataConnector<T>::Publish(OrderBook<T>& _data)
{
    // Not implemented
}

template <typename T>
void MarketDataConnector<T>::Subscribe(ifstream& data)
{
    // Each product ID cycle will process 2 * bookDepth orders
    // (one for BID, one for OFFER, repeated)
    int depthVal = service->GetOrderBookDepth();
    int combinedThreshold = depthVal * 2;
    long orderCount = 0;

    vector<Order> tempBidStack;
    vector<Order> tempOfferStack;

    string lineContent;
    while (getline(data, lineContent))
    {
        if (lineContent.empty()) continue;

        stringstream ss(lineContent);
        string token;
        vector<string> tokens;
        while (getline(ss, token, ','))
        {
            tokens.push_back(token);
        }

        if (tokens.size() < 4) continue;  // Expected: productId, price, quantity, side

        // Parse fields
        string productIdStr = tokens[0];
        double parsedPrice = string2price(tokens[1]);
        long parsedQty = stol(tokens[2]);
        PricingSide parsedSide = (tokens[3] == "BID") ? BID : OFFER;

        // Create order
        Order newOrder(parsedPrice, parsedQty, parsedSide);
        if (parsedSide == BID)
            tempBidStack.push_back(newOrder);
        else
            tempOfferStack.push_back(newOrder);

        ++orderCount;

        // After reading combinedThreshold orders, create an OrderBook and notify
        if (orderCount % combinedThreshold == 0)
        {
            T productObj = GetBond(productIdStr);  // or suitable product creation
            OrderBook<T> assembledBook(productObj, tempBidStack, tempOfferStack);
            service->OnMessage(assembledBook);

            // Reset the stacks for the next batch
            tempBidStack.clear();
            tempOfferStack.clear();
        }
    }
}

#endif // MARKET_DATA_SERVICE_HPP
