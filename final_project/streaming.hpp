/**
 * streaming.hpp
 * Defines the data types for price streams.
 *
 * Author: Breman Thuraisingham
 * Coauthor: Zixiuji Wang
 */
#ifndef STREAMING_HPP
#define STREAMING_HPP

#include "marketdataservice.hpp"

/**
 * PriceStreamOrder
 * Represents a price order with a given price, visible and hidden quantities,
 * and a pricing side (BID/OFFER).
 */
class PriceStreamOrder
{
public:
    // Constructors
    PriceStreamOrder() = default;
    PriceStreamOrder(double p, long visQty, long hidQty, PricingSide s);

    // Accessors
    PricingSide GetSide() const;
    double GetPrice() const;
    long GetVisibleQuantity() const;
    long GetHiddenQuantity() const;

    // Generate a string list for printing/logging
    std::vector<std::string> PrintFunction() const;

private:
    double price;
    long visibleQuantity;
    long hiddenQuantity;
    PricingSide side;
};

/**
 * PriceStream
 * Contains a product T, along with a two-way market: bid and offer orders.
 * Type T is the product type.
 */
template <typename T>
class PriceStream
{
public:
    // Constructors
    PriceStream() = default;
    PriceStream(const T& prod, const PriceStreamOrder& bidOrd, const PriceStreamOrder& offerOrd);

    // Accessors
    const T& GetProduct() const;
    const PriceStreamOrder& GetBidOrder() const;
    const PriceStreamOrder& GetOfferOrder() const;

    // Generate a string representation for printing/logging
    std::vector<std::string> PrintFunction() const;

private:
    T product;
    PriceStreamOrder bidOrder;
    PriceStreamOrder offerOrder;
};

// -------------------- Implementation of PriceStreamOrder --------------------

PriceStreamOrder::PriceStreamOrder(double p, long visQty, long hidQty, PricingSide s)
    : price(p), visibleQuantity(visQty), hiddenQuantity(hidQty), side(s)
{
}

double PriceStreamOrder::GetPrice() const
{
    return price;
}

long PriceStreamOrder::GetVisibleQuantity() const
{
    return visibleQuantity;
}

long PriceStreamOrder::GetHiddenQuantity() const
{
    return hiddenQuantity;
}

PricingSide PriceStreamOrder::GetSide() const
{
    return side;
}

std::vector<std::string> PriceStreamOrder::PrintFunction() const
{
    // Convert numeric values to string
    std::string priceStr   = price2string(price);
    std::string visibleStr = std::to_string(visibleQuantity);
    std::string hiddenStr  = std::to_string(hiddenQuantity);
    std::string sideStr    = (side == BID ? "BID" : "OFFER");

    // Build a vector of these fields
    std::vector<std::string> output{ priceStr, visibleStr, hiddenStr, sideStr };
    return output;
}

// -------------------- Implementation of PriceStream<T> --------------------

template <typename T>
PriceStream<T>::PriceStream(const T& prod, const PriceStreamOrder& bidOrd, const PriceStreamOrder& offerOrd)
    : product(prod), bidOrder(bidOrd), offerOrder(offerOrd)
{
}

template <typename T>
const T& PriceStream<T>::GetProduct() const
{
    return product;
}

template <typename T>
const PriceStreamOrder& PriceStream<T>::GetBidOrder() const
{
    return bidOrder;
}

template <typename T>
const PriceStreamOrder& PriceStream<T>::GetOfferOrder() const
{
    return offerOrder;
}

template <typename T>
std::vector<std::string> PriceStream<T>::PrintFunction() const
{
    // Retrieve the product's ID (assuming T has GetProductId())
    std::string prodIdStr = product.GetProductId();

    // Convert bid/offer orders to string fields
    std::vector<std::string> bidFields   = bidOrder.PrintFunction();
    std::vector<std::string> offerFields = offerOrder.PrintFunction();

    // Concatenate them into a single vector
    std::vector<std::string> output;
    output.push_back(prodIdStr);
    output.insert(output.end(), bidFields.begin(), bidFields.end());
    output.insert(output.end(), offerFields.begin(), offerFields.end());

    return output;
}

#endif // STREAMING_HPP
