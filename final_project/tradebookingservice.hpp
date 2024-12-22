/**
 * tradebookingservice.hpp
 * Defines the data types and Service for trade booking.
 *
 * @author
 *   Breman Thuraisingham
 * @coauthor
 *   Zixiuji Wang
 */

#ifndef TRADE_BOOKING_SERVICE_HPP
#define TRADE_BOOKING_SERVICE_HPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <iostream>
#include "utility.hpp"

// ============================================================================
// ENUM: Side
// ============================================================================
/**
 * Trade sides for a transaction: BUY or SELL.
 */
enum Side
{
    BUY,
    SELL
};

// ============================================================================
// CLASS: Trade<T>
// ============================================================================
/**
 * A Trade object that contains price, side, quantity, and book information.
 * Type T is the product type (e.g., Bond).
 */
template <typename T>
class Trade
{
public:
    // Constructors
    Trade() = default;
    Trade(const T& _product, std::string _tradeId, double _price, std::string _book,
          long _quantity, Side _side);

    // Accessors
    const T& GetProduct() const;
    const std::string& GetTradeId() const;
    double GetPrice() const;
    const std::string& GetBook() const;
    long GetQuantity() const;
    Side GetSide() const;

private:
    T product;
    std::string tradeId;
    double price;
    std::string book;
    long quantity;
    Side side;
};

// -------------------- Implementation of Trade<T> --------------------
template <typename T>
Trade<T>::Trade(const T& _product, std::string _tradeId, double _price,
                std::string _book, long _quantity, Side _side)
    : product(_product), tradeId(_tradeId), price(_price), book(_book),
      quantity(_quantity), side(_side)
{
}

template <typename T>
const T& Trade<T>::GetProduct() const
{
    return product;
}

template <typename T>
const std::string& Trade<T>::GetTradeId() const
{
    return tradeId;
}

template <typename T>
double Trade<T>::GetPrice() const
{
    return price;
}

template <typename T>
const std::string& Trade<T>::GetBook() const
{
    return book;
}

template <typename T>
long Trade<T>::GetQuantity() const
{
    return quantity;
}

template <typename T>
Side Trade<T>::GetSide() const
{
    return side;
}

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
template <typename T>
class TradeBookingConnector;

template <typename T>
class TradeBookingServiceListener;

// ============================================================================
// CLASS: TradeBookingService<T>
// ============================================================================
/**
 * TradeBookingService is responsible for booking trades for a given product.
 * Keyed by trade ID.
 * Type T is the product type.
 */
template <typename T>
class TradeBookingService : public Service<std::string, Trade<T>>
{
public:
    // Constructor
    TradeBookingService();

    // Retrieve data by trade ID
    Trade<T>& GetData(std::string _key) override;

    // Connector callback for new or updated trade data
    void OnMessage(Trade<T>& _data) override;

    // Add a listener to the service
    void AddListener(ServiceListener<Trade<T>>* _listener) override;

    // Retrieve all listeners
    const std::vector<ServiceListener<Trade<T>>*>& GetListeners() const override;

    // Accessor for the connector
    TradeBookingConnector<T>* GetConnector();

    // Accessor for the specialized service listener
    TradeBookingServiceListener<T>* GetListener();

    // Book a trade
    void BookTrade(Trade<T>& tradeObj);

private:
    std::map<std::string, Trade<T>> trades;
    std::vector<ServiceListener<Trade<T>>*> listeners;
    TradeBookingConnector<T>* connector;
    TradeBookingServiceListener<T>* listener;
};

// -------------------- Implementation of TradeBookingService<T> --------------------
template <typename T>
TradeBookingService<T>::TradeBookingService()
    : trades(), listeners(), connector(nullptr), listener(nullptr)
{
    trades = std::map<std::string, Trade<T>>();
    listeners = std::vector<ServiceListener<Trade<T>>*>();

    // Create connector and a specialized service listener
    connector = new TradeBookingConnector<T>(this);
    listener = new TradeBookingServiceListener<T>(this);
}

template <typename T>
Trade<T>& TradeBookingService<T>::GetData(std::string _key)
{
    // Return reference to the trade object in the map
    return trades[_key];
}

template <typename T>
void TradeBookingService<T>::OnMessage(Trade<T>& _data)
{
    // Add or update trade in the map
    trades[_data.GetTradeId()] = _data;

    // Notify all listeners
    for (auto& ls : listeners)
    {
        ls->ProcessAdd(_data);
    }
}

template <typename T>
void TradeBookingService<T>::AddListener(ServiceListener<Trade<T>>* _listener)
{
    listeners.push_back(_listener);
}

template <typename T>
const std::vector<ServiceListener<Trade<T>>*>& TradeBookingService<T>::GetListeners() const
{
    return listeners;
}

template <typename T>
TradeBookingConnector<T>* TradeBookingService<T>::GetConnector()
{
    return connector;
}

template <typename T>
TradeBookingServiceListener<T>* TradeBookingService<T>::GetListener()
{
    return listener;
}

template <typename T>
void TradeBookingService<T>::BookTrade(Trade<T>& tradeObj)
{
    // Notify all listeners about this trade booking
    for (auto& ls : listeners)
    {
        ls->ProcessAdd(tradeObj);
    }
}

// ============================================================================
// CLASS: TradeBookingConnector<T>
// ============================================================================
/**
 * TradeBookingConnector is responsible for subscribing to external data
 * (e.g., "trades.txt") and feeding it to the TradeBookingService.
 * Type T is the product type.
 */
template <typename T>
class TradeBookingConnector : public Connector<Trade<T>>
{
public:
    // Constructor
    TradeBookingConnector(TradeBookingService<T>* _service);

    // Publish data (not used in current design)
    void Publish(Trade<T>& _data) override;

    // Subscribe data from an input stream
    void Subscribe(std::ifstream& _data) override;

private:
    TradeBookingService<T>* bookingService;
};

// -------------------- Implementation of TradeBookingConnector<T> --------------------
template <typename T>
TradeBookingConnector<T>::TradeBookingConnector(TradeBookingService<T>* _service)
    : bookingService(_service)
{
}

template <typename T>
void TradeBookingConnector<T>::Publish(Trade<T>& /*_data*/)
{
    // No-op in this design
}

template <typename T>
void TradeBookingConnector<T>::Subscribe(std::ifstream& inputStream)
{
    // Read lines from "trades.txt" (or another file) and create Trade objects
    std::string lineData;
    while (std::getline(inputStream, lineData))
    {
        if (lineData.empty()) continue;

        std::stringstream lineStream(lineData);
        std::string token;
        std::vector<std::string> fields;

        // Split line by commas
        while (std::getline(lineStream, token, ','))
        {
            fields.push_back(token);
        }
        if (fields.size() < 6) continue;

        // Extract data from fields
        std::string productIdStr = fields[0];
        std::string tradeIdStr = fields[1];
        double parsedPrice = string2price(fields[2]);
        std::string bookStr = fields[3];
        long parsedQty = stol(fields[4]);
        Side tradeSide = (fields[5] == "BUY") ? BUY : SELL;

        // Convert productId to product, e.g., Bond
        T bondProduct = GetBond(productIdStr);

        // Create a Trade object
        Trade<T> newTrade(bondProduct, tradeIdStr, parsedPrice, bookStr,
                          parsedQty, tradeSide);

        // Notify the service
        bookingService->OnMessage(newTrade);
    }
}

// ============================================================================
// CLASS: TradeBookingServiceListener<T>
// ============================================================================
/**
 * TradeBookingServiceListener processes ExecutionOrder<T> events
 * and creates corresponding trades.
 * Type T is the product type.
 */
template <typename T>
class TradeBookingServiceListener : public ServiceListener<ExecutionOrder<T>>
{
public:
    // Constructor
    TradeBookingServiceListener(TradeBookingService<T>* _service);

    // Listener callback to process an add event
    void ProcessAdd(ExecutionOrder<T>& execOrder) override;

    // Listener callbacks (not used here)
    void ProcessRemove(ExecutionOrder<T>& execOrder) override;
    void ProcessUpdate(ExecutionOrder<T>& execOrder) override;

private:
    TradeBookingService<T>* bookingService;
    long bookedCount;
};

// -------------------- Implementation of TradeBookingServiceListener<T> --------------------
template <typename T>
TradeBookingServiceListener<T>::TradeBookingServiceListener(TradeBookingService<T>* _service)
    : bookingService(_service), bookedCount(0)
{
}

template <typename T>
void TradeBookingServiceListener<T>::ProcessAdd(ExecutionOrder<T>& execOrder)
{
    // We maintain a small vector of possible book names
    static std::vector<std::string> marketBooks = { "TRSY1", "TRSY2", "TRSY3" };
    ++bookedCount;

    T productObj = execOrder.GetProduct();
    PricingSide pSide = execOrder.GetPricingSide();
    std::string orderIdStr = execOrder.GetOrderId();
    double orderPrice = execOrder.GetPrice();
    long visibleQty = execOrder.GetVisibleQuantity();
    long hiddenQty = execOrder.GetHiddenQuantity();

    // Convert PricingSide (BID/OFFER) to Trade side (SELL/BUY)
    Side tradeSide;
    if (pSide == BID) tradeSide = SELL;
    else tradeSide = BUY;

    // Determine which book in round-robin style
    std::string chosenBook = marketBooks[bookedCount % 3];
    long totalQty = visibleQty + hiddenQty;

    // Create a Trade object
    Trade<T> generatedTrade(productObj, orderIdStr, orderPrice,
                            chosenBook, totalQty, tradeSide);

    // Pass the trade to the booking service
    bookingService->OnMessage(generatedTrade);
    bookingService->BookTrade(generatedTrade);
}

template <typename T>
void TradeBookingServiceListener<T>::ProcessRemove(ExecutionOrder<T>& /*execOrder*/)
{
    // Not used
}

template <typename T>
void TradeBookingServiceListener<T>::ProcessUpdate(ExecutionOrder<T>& /*execOrder*/)
{
    // Not used
}

#endif // TRADE_BOOKING_SERVICE_HPP
