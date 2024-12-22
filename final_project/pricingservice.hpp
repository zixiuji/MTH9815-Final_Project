/**
 * pricingservice.hpp
 * Defines the data types and Service for internal prices.
 *
 * @author
 *   Breman Thuraisingham
 * @coauthor
 *   Zixiuji Wang
 */

#define _CRT_SECURE_NO_WARNINGS 1

#ifndef PRICING_SERVICE_HPP
#define PRICING_SERVICE_HPP

#include "utility.hpp"
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

// ============================================================================
// PRICE CLASS
// ============================================================================

/**
 * A price object consisting of mid and bid/offer spread.
 * Type T is the product type.
 */
template <typename T>
class Price {
public:
    // Default constructor
    Price() = default;

    // Constructor with product, mid, and bid-offer spread
    Price(const T& _product, double _mid, double _bidOfferSpread);

    // Accessors
    const T& GetProduct() const;
    double GetMid() const;
    double GetBidOfferSpread() const;

    // Convert attributes to strings (for printing or logging)
    std::vector<std::string> ToStrings() const;
    std::vector<std::string> PrintFunction() const;

private:
    // Private data members
    T product;
    double mid;
    double bidOfferSpread;
};


// ----------------- Implementation of Price<T> -----------------

template <typename T>
Price<T>::Price(const T& _product, double _mid, double _bidOfferSpread)
    : product(_product), mid(_mid), bidOfferSpread(_bidOfferSpread)
{
}

template <typename T>
const T& Price<T>::GetProduct() const
{
    return product;
}

template <typename T>
double Price<T>::GetMid() const
{
    return mid;
}

template <typename T>
double Price<T>::GetBidOfferSpread() const
{
    return bidOfferSpread;
}

template <typename T>
std::vector<std::string> Price<T>::ToStrings() const
{
    // If you need different string representations, you can modify here.
    return PrintFunction();
}

template <typename T>
std::vector<std::string> Price<T>::PrintFunction() const
{
    std::string productIdStr = product.GetProductId();
    std::string midStr = price2string(mid);
    std::string spreadStr = price2string(bidOfferSpread);

    std::vector<std::string> outputVec;
    outputVec.push_back(productIdStr);
    outputVec.push_back(midStr);
    outputVec.push_back(spreadStr);
    return outputVec;
}

// ============================================================================
// FORWARD DECLARATION
// ============================================================================
template <typename T>
class PricingConnector;

// ============================================================================
// PRICING SERVICE
// ============================================================================
/**
 * PricingService manages mid prices and bid-offer spreads for products.
 * Keyed by product identifier (std::string).
 */
template <typename T>
class PricingService : public Service<std::string, Price<T>>
{
public:
    // Constructor / Destructor
    PricingService();
    ~PricingService();

    // Retrieve data by product ID
    Price<T>& GetData(std::string _key);

    // Callback for new or updated price data
    void OnMessage(Price<T>& _data);

    // Add a service listener
    void AddListener(ServiceListener<Price<T>>* listener);

    // Retrieve all listeners
    const std::vector<ServiceListener<Price<T>>*>& GetListeners() const;

    // Obtain the associated connector
    PricingConnector<T>* GetConnector();

private:
    // Internal container storing prices by product ID
    std::map<std::string, Price<T>> internalPriceMap;

    // List of service listeners
    std::vector<ServiceListener<Price<T>>*> priceSrvListeners;

    // Connector for this pricing service
    PricingConnector<T>* pricingConn;
};

// ============================================================================
// PRICING CONNECTOR
// ============================================================================
template <typename T>
class PricingConnector : public Connector<Price<T>>
{
public:
    // Constructor
    PricingConnector(PricingService<T>* srvPtr);

    // Publish data to the connector (not used in this design)
    void Publish(Price<T>& _data) override;

    // Subscribe data from an input stream
    void Subscribe(std::ifstream& _data) override;

private:
    // Pointer back to the associated PricingService
    PricingService<T>* serviceRef;
};

// ============================================================================
// IMPLEMENTATION OF PRICING SERVICE
// ============================================================================
template <typename T>
PricingService<T>::PricingService()
    : internalPriceMap(), priceSrvListeners(), pricingConn(nullptr)
{
    // Initialize internal structures
    internalPriceMap = std::map<std::string, Price<T>>();
    priceSrvListeners = std::vector<ServiceListener<Price<T>>*>();
    pricingConn = new PricingConnector<T>(this);
}

template <typename T>
PricingService<T>::~PricingService()
{
    // If needed, clean up resources
}

template <typename T>
Price<T>& PricingService<T>::GetData(std::string _key)
{
    // Return reference to the price object in the map
    return internalPriceMap[_key];
}

template <typename T>
void PricingService<T>::OnMessage(Price<T>& _data)
{
    // Insert or update the price in the map
    std::string pid = _data.GetProduct().GetProductId();
    internalPriceMap[pid] = _data;  // Overwrites existing if present

    // Notify all listeners of a new Add event
    for (auto& lst : priceSrvListeners)
    {
        lst->ProcessAdd(_data);
    }
}

template <typename T>
void PricingService<T>::AddListener(ServiceListener<Price<T>>* listener)
{
    priceSrvListeners.push_back(listener);
}

template <typename T>
const std::vector<ServiceListener<Price<T>>*>& PricingService<T>::GetListeners() const
{
    return priceSrvListeners;
}

template <typename T>
PricingConnector<T>* PricingService<T>::GetConnector()
{
    return pricingConn;
}

// ============================================================================
// IMPLEMENTATION OF PRICING CONNECTOR
// ============================================================================
template <typename T>
PricingConnector<T>::PricingConnector(PricingService<T>* srvPtr)
    : serviceRef(srvPtr)
{
}

template <typename T>
void PricingConnector<T>::Publish(Price<T>& _data)
{
    // Not implemented in this design
}

template <typename T>
void PricingConnector<T>::Subscribe(std::ifstream& _data)
{
    // Read each line from the ifstream
    std::string singleLine;
    while (std::getline(_data, singleLine))
    {
        if (singleLine.empty()) continue;

        std::stringstream lineStream(singleLine);
        std::string segment;
        std::vector<std::string> parsedFields;

        // Split by comma
        while (std::getline(lineStream, segment, ','))
        {
            parsedFields.push_back(segment);
        }

        // Expect at least 3 columns: productId, bid_price, offer_price
        if (parsedFields.size() < 3) continue;

        // Extract fields
        std::string _productId = parsedFields[0];
        double bidVal = string2price(parsedFields[1]);
        double offerVal = string2price(parsedFields[2]);

        // Convert productId to product object, e.g. Bond
        T bondObj = GetBond(_productId);

        // Calculate mid and spread
        double midVal = (bidVal + offerVal) / 2.0;
        double spreadVal = offerVal - bidVal;

        // Create a new Price<T> object
        Price<T> priceObj(bondObj, midVal, spreadVal);

        // Pass the price to the service
        serviceRef->OnMessage(priceObj);
    }
}

#endif // PRICING_SERVICE_HPP
