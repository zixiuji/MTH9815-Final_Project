/**
 * AlgoStreamingService.hpp
 * Defines the data types and Service for price streams.
 *
 * Author: Breman Thuraisingham
 * Coauthor: Zixiuji Wang
 */
#ifndef ALGO_STREAMING_SERVICE_HPP
#define ALGO_STREAMING_SERVICE_HPP

#include "streaming.hpp"
#include "pricingservice.hpp"
#include "utility.hpp"

/**
 * AlgoStream
 * Encapsulates a PriceStream<T> for algorithmic streaming decisions.
 * Type T is the product type.
 */
template<typename T>
class AlgoStream
{
public:
    // Constructors
    AlgoStream() = default;
    AlgoStream(const T& prod, const PriceStreamOrder& bidOrd, const PriceStreamOrder& offerOrd);

    // Retrieve the underlying PriceStream
    PriceStream<T>* GetPriceStream() const;

private:
    PriceStream<T>* priceStream;
};

// -------------------- Implementation of AlgoStream<T> --------------------

template<typename T>
AlgoStream<T>::AlgoStream(const T& prod, const PriceStreamOrder& bidOrd, const PriceStreamOrder& offerOrd)
{
    priceStream = new PriceStream<T>(prod, bidOrd, offerOrd);
}

template<typename T>
PriceStream<T>* AlgoStream<T>::GetPriceStream() const
{
    return priceStream;
}

// ---------------------------------------------------------------------------
// Forward declarations
template<typename T>
class AlgoStreamingServiceListener;

/**
 * AlgoStreamingService
 * Maintains AlgoStream<T> keyed by product ID (string).
 * Type T is the product type (e.g. Bond).
 */
template<typename T>
class AlgoStreamingService : public Service<std::string, AlgoStream<T>>
{
public:
    // Constructors / Destructor
    AlgoStreamingService();
    ~AlgoStreamingService();

    // Retrieve an AlgoStream<T> by product ID
    AlgoStream<T>& GetData(std::string key) override;

    // Connector/Listener callback for new or updated data
    void OnMessage(AlgoStream<T>& dataObj) override;

    // Add a service listener
    void AddListener(ServiceListener<AlgoStream<T>>* listenerPtr) override;

    // Retrieve all service listeners
    const std::vector<ServiceListener<AlgoStream<T>>*>& GetListeners() const override;

    // Return the specialized listener (for Price<T>)
    ServiceListener<Price<T>>* GetListener();

    // Publish a two-way price based on the input Price<T>
    void AlgoPublishPrice(Price<T>& priceObj);

private:
    std::map<std::string, AlgoStream<T>> algoStreams;
    std::vector<ServiceListener<AlgoStream<T>>*> listeners;
    ServiceListener<Price<T>>* listener;
    long pricePublishCount;
};

// -------------------- Implementation of AlgoStreamingService<T> --------------------

template<typename T>
AlgoStreamingService<T>::AlgoStreamingService()
{
    algoStreams       = std::map<std::string, AlgoStream<T>>();
    listeners         = std::vector<ServiceListener<AlgoStream<T>>*>();
    listener          = new AlgoStreamingServiceListener<T>(this);
    pricePublishCount = 0;
}

template<typename T>
AlgoStreamingService<T>::~AlgoStreamingService()
{
    // Default destructor
}

template<typename T>
AlgoStream<T>& AlgoStreamingService<T>::GetData(std::string key)
{
    return algoStreams[key];
}

template<typename T>
void AlgoStreamingService<T>::OnMessage(AlgoStream<T>& dataObj)
{
    // Update or store the AlgoStream keyed by the product ID
    std::string prodId = dataObj.GetPriceStream()->GetProduct().GetProductId();
    algoStreams[prodId] = dataObj;
}

template<typename T>
void AlgoStreamingService<T>::AddListener(ServiceListener<AlgoStream<T>>* listenerPtr)
{
    listeners.push_back(listenerPtr);
}

template<typename T>
const std::vector<ServiceListener<AlgoStream<T>>*>&
AlgoStreamingService<T>::GetListeners() const
{
    return listeners;
}

template<typename T>
ServiceListener<Price<T>>* AlgoStreamingService<T>::GetListener()
{
    return listener;
}

/**
 * AlgoPublishPrice
 * Generates bid/offer PriceStreamOrders from the given Price<T> and
 * stores them as an AlgoStream, then notifies listeners.
 */
template<typename T>
void AlgoStreamingService<T>::AlgoPublishPrice(Price<T>& priceObj)
{
    // Extract product
    T productRef  = priceObj.GetProduct();
    std::string prodId = productRef.GetProductId();

    double midVal    = priceObj.GetMid();
    double spreadVal = priceObj.GetBidOfferSpread();

    // Calculate bid & offer
    double bidVal   = midVal - spreadVal / 2.0;
    double offerVal = midVal + spreadVal / 2.0;

    // Alternate visible/hide quantities
    long visQty = (pricePublishCount % 2 + 1) * 1000000;
    long hidQty = visQty * 2;

    pricePublishCount++;

    // Build PriceStreamOrder
    PriceStreamOrder bidOrd(bidVal, visQty, hidQty, BID);
    PriceStreamOrder offerOrd(offerVal, visQty, hidQty, OFFER);

    // Build AlgoStream
    AlgoStream<T> algoStr(productRef, bidOrd, offerOrd);
    algoStreams[prodId] = algoStr;

    // Notify all listeners with new AlgoStream
    for (auto& ls : listeners)
    {
        ls->ProcessAdd(algoStr);
    }
}

/**
 * AlgoStreamingServiceListener
 * Subscribes to Price<T> from the PricingService and triggers algo-based publishing.
 */
template<typename T>
class AlgoStreamingServiceListener : public ServiceListener<Price<T>>
{
public:
    // Constructor
    explicit AlgoStreamingServiceListener(AlgoStreamingService<T>* svc);

    // Listener callbacks
    void ProcessAdd(Price<T>& dataObj) override;
    void ProcessRemove(Price<T>& dataObj) override;
    void ProcessUpdate(Price<T>& dataObj) override;

private:
    AlgoStreamingService<T>* service;
};

// -------------------- Implementation of AlgoStreamingServiceListener<T> --------------------

template<typename T>
AlgoStreamingServiceListener<T>::AlgoStreamingServiceListener(AlgoStreamingService<T>* svc)
    : service(svc)
{
}

template<typename T>
void AlgoStreamingServiceListener<T>::ProcessAdd(Price<T>& dataObj)
{
    // Publish price via AlgoStreamingService
    service->AlgoPublishPrice(dataObj);
}

template<typename T>
void AlgoStreamingServiceListener<T>::ProcessRemove(Price<T>& /*dataObj*/)
{
    // Not used
}

template<typename T>
void AlgoStreamingServiceListener<T>::ProcessUpdate(Price<T>& /*dataObj*/)
{
    // Not used
}

#endif // ALGO_STREAMING_SERVICE_HPP
