/**
 * execution.hpp
 * Defines the ExecutionOrder data type for executions.
 *
 * Author: Breman Thuraisingham
 * Coauthor: Zixiuji Wang
 */
#ifndef EXECUTION_HPP
#define EXECUTION_HPP

#include <string>
#include "marketdataservice.hpp"

// Enumeration for order types
enum OrderType { FOK, IOC, MARKET, LIMIT, STOP };

// Enumeration for different markets
enum Market { BROKERTEC, ESPEED, CME };

/**
 * ExecutionOrder
 * Represents an order that can be executed on an exchange.
 * Type T is the product type (e.g., Bond).
 */
template <typename T>
class ExecutionOrder
{
public:
    // Constructors
    ExecutionOrder() = default;
    ExecutionOrder(const T& prod, PricingSide ps, std::string oid,
                   OrderType ot, double pr, double visQty, double hidQty,
                   std::string parentOid, bool childFlag);

    // Accessors
    const T& GetProduct() const;
    PricingSide GetPricingSide() const;
    const std::string& GetOrderId() const;
    OrderType GetOrderType() const;
    double GetPrice() const;
    long GetVisibleQuantity() const;
    long GetHiddenQuantity() const;
    const std::string& GetParentOrderId() const;
    bool IsChildOrder() const;

    // Generate a list of string fields for printing/logging
    std::vector<std::string> PrintFunction() const;

private:
    T product;
    PricingSide side;
    std::string orderId;
    OrderType orderType;
    double price;
    long visibleQuantity;
    long hiddenQuantity;
    std::string parentOrderId;
    bool isChildOrder;
};

// -------------------- Implementation of ExecutionOrder<T> --------------------

template <typename T>
ExecutionOrder<T>::ExecutionOrder(const T& prod, PricingSide ps,
                                  std::string oid, OrderType ot,
                                  double pr, double visQty, double hidQty,
                                  std::string parentOid, bool childFlag)
    : product(prod)
{
    side           = ps;
    orderId        = oid;
    orderType      = ot;
    price          = pr;
    visibleQuantity = static_cast<long>(visQty);
    hiddenQuantity  = static_cast<long>(hidQty);
    parentOrderId  = parentOid;
    isChildOrder   = childFlag;
}

template <typename T>
const T& ExecutionOrder<T>::GetProduct() const
{
    return product;
}

template <typename T>
PricingSide ExecutionOrder<T>::GetPricingSide() const
{
    return side;
}

template <typename T>
const std::string& ExecutionOrder<T>::GetOrderId() const
{
    return orderId;
}

template <typename T>
OrderType ExecutionOrder<T>::GetOrderType() const
{
    return orderType;
}

template <typename T>
double ExecutionOrder<T>::GetPrice() const
{
    return price;
}

template <typename T>
long ExecutionOrder<T>::GetVisibleQuantity() const
{
    return visibleQuantity;
}

template <typename T>
long ExecutionOrder<T>::GetHiddenQuantity() const
{
    return hiddenQuantity;
}

template <typename T>
const std::string& ExecutionOrder<T>::GetParentOrderId() const
{
    return parentOrderId;
}

template <typename T>
bool ExecutionOrder<T>::IsChildOrder() const
{
    return isChildOrder;
}

template <typename T>
std::vector<std::string> ExecutionOrder<T>::PrintFunction() const
{
    // Convert internal data to string representations
    std::string prodIdStr   = product.GetProductId();
    std::string sideStr     = (side == BID) ? "BID" : "OFFER";
    std::string oidStr      = orderId;

    // Map the order type enum to string
    std::string otStr;
    switch (orderType)
    {
        case FOK:    otStr = "FOK";    break;
        case IOC:    otStr = "IOC";    break;
        case MARKET: otStr = "MARKET"; break;
        case LIMIT:  otStr = "LIMIT";  break;
        case STOP:   otStr = "STOP";   break;
        default:     otStr = "UNKNOWN";
    }

    // Convert numeric fields
    std::string priceStr = price2string(price);

    // Convert quantity to string but ensure it's integral
    std::string visQtyStr = std::to_string(visibleQuantity);
    std::string hidQtyStr = std::to_string(hiddenQuantity);

    // Parent order ID and child flag
    std::string parentStr = parentOrderId;
    std::string childFlagStr = isChildOrder ? "YES" : "NO";

    // Build the list of fields
    std::vector<std::string> output {
        prodIdStr,
        sideStr,
        oidStr,
        otStr,
        priceStr,
        visQtyStr,
        hidQtyStr,
        parentStr,
        childFlagStr
    };
    return output;
}

#endif // EXECUTION_HPP
