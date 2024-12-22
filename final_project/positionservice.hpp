/**
 * positionservice.hpp
 * Defines the data types and Service for positions.
 *
 * Author: Breman Thuraisingham
 * Coauthor: Zixiuji Wang
 */

#ifndef POSITION_SERVICE_HPP
#define POSITION_SERVICE_HPP

#include "tradebookingservice.hpp"
#include <map>
#include <string>

using namespace std;

/**
 * Position class representing holdings in a particular book.
 * Type T is the product type (e.g., Bond).
 */
template <typename T>
class Position
{
public:
    // Constructors
    Position() = default;
    explicit Position(const T& prod);

    // Return the product associated with this position
    const T& GetProduct() const;

    // Return the position quantity for a specific book
    long GetPosition(const std::string& book);

    // Return the map of (book -> quantity)
    map<string, long> GetPositions();

    // Add a position quantity to a specific book
    void AddPosition(const std::string& book, long qty);

    // Compute the aggregate position across all books
    long GetAggregatePosition();

    // Generate a list of strings for logging/printing
    vector<string> PrintFunction() const;

private:
    T product;                // The product (e.g. a Bond)
    map<string, long> bookPositions; // Key: book name, Value: quantity
};

// -------------------- Implementation of Position<T> --------------------

template <typename T>
Position<T>::Position(const T& prod)
    : product(prod)
{
}

template <typename T>
const T& Position<T>::GetProduct() const
{
    return product;
}

template <typename T>
long Position<T>::GetPosition(const std::string& book)
{
    return bookPositions[book];
}

template <typename T>
map<string, long> Position<T>::GetPositions()
{
    return bookPositions;
}

template <typename T>
void Position<T>::AddPosition(const std::string& book, long qty)
{
    bookPositions[book] += qty;
}

template <typename T>
long Position<T>::GetAggregatePosition()
{
    long total = 0;
    for (auto& kv : bookPositions)
    {
        total += kv.second;
    }
    return total;
}

template <typename T>
vector<string> Position<T>::PrintFunction() const
{
    // First element: product ID
    vector<string> output;
    output.push_back(product.GetProductId());

    // Then each book name + quantity
    for (auto& kv : bookPositions)
    {
        output.push_back(kv.first);
        output.push_back(to_string(kv.second));
    }
    return output;
}

// -------------------------------------------------------------------------
// Forward declarations
template <typename T> class PositionServiceListener;

/**
 * PositionService
 * Manages positions across multiple books for a given product type T.
 * Keyed on product identifier (std::string).
 */
template <typename T>
class PositionService : public Service<string, Position<T>>
{
public:
    // Constructor
    PositionService();

    // Retrieve a Position<T> by product ID
    Position<T>& GetData(string key) override;

    // Callback for receiving new/updated Position data
    void OnMessage(Position<T>& data) override;

    // Add a service listener
    void AddListener(ServiceListener<Position<T>>* listenerPtr) override;

    // Retrieve all service listeners
    const vector<ServiceListener<Position<T>>*>& GetListeners() const override;

    // Retrieve the specialized listener
    PositionServiceListener<T>* GetListener();

    // Add a trade (from TradeBookingService) to update positions
    virtual void AddTrade(const Trade<T>& tradeObj);

private:
    map<string, Position<T>> positions;
    vector<ServiceListener<Position<T>>*> listeners;
    PositionServiceListener<T>* listener;
};

// -------------------- Implementation of PositionService<T> --------------------

template <typename T>
PositionService<T>::PositionService()
{
    positions = map<string, Position<T>>();
    listeners = vector<ServiceListener<Position<T>>*>();
    listener  = new PositionServiceListener<T>(this);
}

template <typename T>
Position<T>& PositionService<T>::GetData(string key)
{
    return positions[key];
}

template <typename T>
void PositionService<T>::OnMessage(Position<T>& data)
{
    string prodId = data.GetProduct().GetProductId();
    positions[prodId] = data;
}

template <typename T>
void PositionService<T>::AddListener(ServiceListener<Position<T>>* listenerPtr)
{
    listeners.push_back(listenerPtr);
}

template <typename T>
PositionServiceListener<T>* PositionService<T>::GetListener()
{
    return listener;
}

template <typename T>
const vector<ServiceListener<Position<T>>*>& PositionService<T>::GetListeners() const
{
    return listeners;
}

template <typename T>
void PositionService<T>::AddTrade(const Trade<T>& tradeObj)
{
    // Extract details
    T prod       = tradeObj.GetProduct();
    string prodId  = prod.GetProductId();
    string bookName = tradeObj.GetBook();
    long tradeQty   = tradeObj.GetQuantity();
    Side side       = tradeObj.GetSide();

    // If not present, create a new Position
    Position<T> newPosition(prod);

    // Add or subtract based on side
    if (side == BUY)
        newPosition.AddPosition(bookName, tradeQty);
    else
        newPosition.AddPosition(bookName, -tradeQty);

    // Merge with any existing data
    Position<T> oldPosition = positions[prodId];
    map<string, long> oldMap = oldPosition.GetPositions();
    for (auto& kv : oldMap)
    {
        newPosition.AddPosition(kv.first, kv.second);
    }
    positions[prodId] = newPosition;

    // Notify listeners
    for (auto& ls : listeners)
    {
        ls->ProcessAdd(newPosition);
    }
}

// -------------------------------------------------------------------------
// CLASS: PositionServiceListener<T>
/**
 * PositionServiceListener listens for Trade<T> events from the TradeBookingService
 * and updates the PositionService accordingly.
 */
template <typename T>
class PositionServiceListener : public ServiceListener<Trade<T>>
{
public:
    // Constructor
    explicit PositionServiceListener(PositionService<T>* svc);

    // Required callbacks
    void ProcessAdd(Trade<T>& data) override;
    void ProcessRemove(Trade<T>& data) override;
    void ProcessUpdate(Trade<T>& data) override;

private:
    PositionService<T>* service;
};

// -------------------- Implementation of PositionServiceListener<T> --------------------

template <typename T>
PositionServiceListener<T>::PositionServiceListener(PositionService<T>* svc)
    : service(svc)
{
}

template <typename T>
void PositionServiceListener<T>::ProcessAdd(Trade<T>& data)
{
    // Each new trade modifies positions
    service->AddTrade(data);
}

template <typename T>
void PositionServiceListener<T>::ProcessRemove(Trade<T>& /*data*/)
{
    // Not implemented
}

template <typename T>
void PositionServiceListener<T>::ProcessUpdate(Trade<T>& /*data*/)
{
    // Not implemented
}

#endif // POSITION_SERVICE_HPP
