/**
 * streamingservice.hpp
 * Defines the Service and Listener for price streams.
 *
 * Author: Breman Thuraisingham
 * Coauthor: Zixiuji Wang
 */
#ifndef STREAMING_SERVICE_HPP
#define STREAMING_SERVICE_HPP

#include "soa.hpp"
#include "streaming.hpp"
#include "AlgoStreamingService.hpp"

// Forward declarations to avoid errors.
template<typename T>
class StreamingServiceListener;

/**
 * StreamingService
 * Publishes two-way prices keyed by product identifier.
 * Type T is the product type (e.g., Bond).
 */
template<typename T>
class StreamingService : public Service<std::string, PriceStream<T>>
{
public:
    // Constructor
    StreamingService();

    // Retrieve the PriceStream for a given product ID
    PriceStream<T>& GetData(std::string key) override;

    // Callback for new or updated data from a connector
    void OnMessage(PriceStream<T>& dataObj) override;

    // Add a listener for PriceStream<T> events
    void AddListener(ServiceListener<PriceStream<T>>* listenerPtr) override;

    // Retrieve all registered listeners
    const std::vector<ServiceListener<PriceStream<T>>*>& GetListeners() const override;

    // Retrieve the specialized listener
    ServiceListener<AlgoStream<T>>* GetListener();

    // Publish two-way prices
    void PublishPrice(PriceStream<T>& priceStreamObj);

private:
    std::map<std::string, PriceStream<T>> priceStreams;
    std::vector<ServiceListener<PriceStream<T>>*> listeners;
    ServiceListener<AlgoStream<T>>* listener;
};

// -------------------- Implementation of StreamingService<T> --------------------

template<typename T>
StreamingService<T>::StreamingService()
{
    priceStreams = std::map<std::string, PriceStream<T>>();
    listeners    = std::vector<ServiceListener<PriceStream<T>>*>();
    listener     = new StreamingServiceListener<T>(this);
}

template<typename T>
PriceStream<T>& StreamingService<T>::GetData(std::string key)
{
    return priceStreams[key];
}

template<typename T>
void StreamingService<T>::OnMessage(PriceStream<T>& dataObj)
{
    std::string prodId = dataObj.GetProduct().GetProductId();
    priceStreams[prodId] = dataObj;
}

template<typename T>
void StreamingService<T>::AddListener(ServiceListener<PriceStream<T>>* listenerPtr)
{
    listeners.push_back(listenerPtr);
}

template<typename T>
const std::vector<ServiceListener<PriceStream<T>>*>& StreamingService<T>::GetListeners() const
{
    return listeners;
}

template<typename T>
ServiceListener<AlgoStream<T>>* StreamingService<T>::GetListener()
{
    return listener;
}

template<typename T>
void StreamingService<T>::PublishPrice(PriceStream<T>& priceStreamObj)
{
    // Notify each listener about the new or updated PriceStream
    for (auto& ls : listeners)
    {
        ls->ProcessAdd(priceStreamObj);
    }
}

// -------------------------------------------------------------------------
// CLASS: StreamingServiceListener<T>
/**
 * StreamingServiceListener
 * Subscribes to AlgoStream<T> data from BondAlgoExecutionService
 * and updates the StreamingService with new PriceStreams.
 */
template<typename T>
class StreamingServiceListener : public ServiceListener<AlgoStream<T>>
{
public:
    // Constructor
    explicit StreamingServiceListener(StreamingService<T>* svc);

    // Destructor (no override needed if base not virtual)
    ~StreamingServiceListener();

    // Listener callbacks
    void ProcessAdd(AlgoStream<T>& dataObj) override;
    void ProcessRemove(AlgoStream<T>& dataObj) override;
    void ProcessUpdate(AlgoStream<T>& dataObj) override;

private:
    StreamingService<T>* service;
};

// -------------------- Implementation of StreamingServiceListener<T> --------------------

template<typename T>
StreamingServiceListener<T>::StreamingServiceListener(StreamingService<T>* svc)
    : service(svc)
{
}

template<typename T>
StreamingServiceListener<T>::~StreamingServiceListener()
{
    // Default destructor
}

template<typename T>
void StreamingServiceListener<T>::ProcessAdd(AlgoStream<T>& dataObj)
{
    // Extract the PriceStream from AlgoStream
    PriceStream<T>* priceStreamPtr = dataObj.GetPriceStream();

    // Use OnMessage to store it in StreamingService
    service->OnMessage(*priceStreamPtr);

    // Then publish it
    service->PublishPrice(*priceStreamPtr);
}

template<typename T>
void StreamingServiceListener<T>::ProcessRemove(AlgoStream<T>& /*dataObj*/)
{
    // Not implemented
}

template<typename T>
void StreamingServiceListener<T>::ProcessUpdate(AlgoStream<T>& /*dataObj*/)
{
    // Not implemented
}

#endif // STREAMING_SERVICE_HPP
