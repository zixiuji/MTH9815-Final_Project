/**
 * historicaldataservice.hpp
 *
 * Defines the HistoricalDataService template class for historical data processing and persistence.
 * This service is used to process and persist historical data keyed on a persistent key.
 *
 * @author
 *   Breman Thuraisingham
 * @coauthor
 *   Zixiuji Wang
 */

#ifndef HISTORICAL_DATA_SERVICE_HPP
#define HISTORICAL_DATA_SERVICE_HPP

#include "executionservice.hpp"
#include "inquiryservice.hpp"
#include "positionservice.hpp"
#include "riskservice.hpp"
#include "soa.hpp"
#include "streamingservice.hpp"
#include "utility.hpp"
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <iostream>

// Forward declarations
template <typename V> class HistoricalDataConnector;
template <typename V> class HistoricalDataListener;

/**
 * HistoricalDataService
 * A service for processing and persisting historical data keyed by a string.
 * Type V is the data type to persist (e.g., Position<Bond>, PV01<Bond>, etc.).
 */
template <typename V>
class HistoricalDataService : public Service<std::string, V>
{
public:
    // Constructors
    HistoricalDataService();
    explicit HistoricalDataService(std::string _type);

    // Retrieve data by key
    V& GetData(std::string key) override;

    // Called by connector with new or updated data
    void OnMessage(V& dataObj) override;

    // Add a listener
    void AddListener(ServiceListener<V>* listenerPtr) override;

    // Retrieve all listeners
    const std::vector<ServiceListener<V>*>& GetListeners() const override;

    // Access the connector
    HistoricalDataConnector<V>* GetConnector();

    // Access the service listener
    ServiceListener<V>* GetServiceListener();

    // Service type (e.g. "Position", "Risk", "Execution", "Streaming", "Inquiry")
    std::string GetServiceType() const;

    // Persist data using the connector
    void PersistData(std::string persistKey, V& dataObj);

private:
    std::map<std::string, V> historicalDataMap;
    std::vector<ServiceListener<V>*> serviceListeners;
    HistoricalDataConnector<V>* dataConnector;
    ServiceListener<V>* dataListener;
    std::string serviceType;
};

// --------------------------------------------------------------------------
// Implementation of HistoricalDataService<V>
// --------------------------------------------------------------------------
template <typename V>
HistoricalDataService<V>::HistoricalDataService()
    : historicalDataMap(),
      serviceListeners(),
      dataConnector(nullptr),
      dataListener(nullptr),
      serviceType("Position")
{
    // Initialize containers
    historicalDataMap = std::map<std::string, V>();
    serviceListeners = std::vector<ServiceListener<V>*>();

    // Create connector and listener
    dataConnector = new HistoricalDataConnector<V>(this);
    dataListener = new HistoricalDataListener<V>(this);
}

template <typename V>
HistoricalDataService<V>::HistoricalDataService(std::string _type)
    : historicalDataMap(),
      serviceListeners(),
      dataConnector(nullptr),
      dataListener(nullptr),
      serviceType(std::move(_type))
{
    // Initialize containers
    historicalDataMap = std::map<std::string, V>();
    serviceListeners = std::vector<ServiceListener<V>*>();

    // Create connector and listener
    dataConnector = new HistoricalDataConnector<V>(this);
    dataListener = new HistoricalDataListener<V>(this);
}

template <typename V>
V& HistoricalDataService<V>::GetData(std::string key)
{
    return historicalDataMap[key];
}

template <typename V>
void HistoricalDataService<V>::OnMessage(V& dataObj)
{
    // Store or update data in the map, keyed by the product ID
    historicalDataMap[dataObj.GetProduct().GetProductId()] = dataObj;
}

template <typename V>
void HistoricalDataService<V>::AddListener(ServiceListener<V>* listenerPtr)
{
    serviceListeners.push_back(listenerPtr);
}

template <typename V>
const std::vector<ServiceListener<V>*>& HistoricalDataService<V>::GetListeners() const
{
    return serviceListeners;
}

template <typename V>
HistoricalDataConnector<V>* HistoricalDataService<V>::GetConnector()
{
    return dataConnector;
}

template <typename V>
ServiceListener<V>* HistoricalDataService<V>::GetServiceListener()
{
    return dataListener;
}

template <typename V>
std::string HistoricalDataService<V>::GetServiceType() const
{
    return serviceType;
}

template <typename V>
void HistoricalDataService<V>::PersistData(std::string persistKey, V& dataObj)
{
    // Delegate the actual writing to the connector
    dataConnector->Publish(dataObj);
}

// ============================================================================
// CLASS: HistoricalDataConnector<V>
// ============================================================================
/**
 * HistoricalDataConnector handles publishing data to local files
 * based on the service type. Subscribe is unused here.
 */
template <typename V>
class HistoricalDataConnector : public Connector<V>
{
public:
    // Constructor
    explicit HistoricalDataConnector(HistoricalDataService<V>* svcPtr);

    // Publish data (write to file)
    void Publish(V& dataObj) override;

    // Subscribe data (not implemented)
    void Subscribe(std::ifstream& fileStream) override;

private:
    HistoricalDataService<V>* serviceRef;
};

// --------------------------------------------------------------------------
// Implementation of HistoricalDataConnector<V>
// --------------------------------------------------------------------------
template <typename V>
HistoricalDataConnector<V>::HistoricalDataConnector(HistoricalDataService<V>* svcPtr)
    : serviceRef(svcPtr)
{
}

template <typename V>
void HistoricalDataConnector<V>::Publish(V& dataObj)
{
    std::string typeStr = serviceRef->GetServiceType();
    std::ofstream outFile;

    // Determine which file to open based on service type
    if (typeStr == "Position")
    {
        outFile.open("Data/Output/positions.txt", std::ios::app);
        if (!outFile.is_open())
        {
            std::cerr << "Error: Unable to open file Data/Output/positions.txt\n";
            return;
        }
    }
    else if (typeStr == "Risk")
    {
        outFile.open("Data/Output/risk.txt", std::ios::app);
        if (!outFile.is_open())
        {
            std::cerr << "Error: Unable to open file Data/Output/risk.txt\n";
            return;
        }
    }
    else if (typeStr == "Execution")
    {
        outFile.open("Data/Output/executions.txt", std::ios::app);
        if (!outFile.is_open())
        {
            std::cerr << "Error: Unable to open file Data/Output/executions.txt\n";
            return;
        }
    }
    else if (typeStr == "Streaming")
    {
        outFile.open("Data/Output/streaming.txt", std::ios::app);
        if (!outFile.is_open())
        {
            std::cerr << "Error: Unable to open file Data/Output/streaming.txt\n";
            return;
        }
    }
    else if (typeStr == "Inquiry")
    {
        outFile.open("Data/Output/allinquiries.txt", std::ios::app);
        if (!outFile.is_open())
        {
            std::cerr << "Error: Unable to open file Data/Output/allinquiries.txt\n";
            return;
        }
    }
    else
    {
        // If service type is unknown, write to a fallback file
        outFile.open("Data/Output/unknown.txt", std::ios::app);
        if (!outFile.is_open())
        {
            std::cerr << "Error: Unable to open file Data/Output/unknown.txt\n";
            return;
        }
    }

    // Write timestamp
    auto nowTime = microsec_clock::local_time();
    outFile << nowTime << ",";

    // Write data fields
    std::vector<std::string> dataFields = dataObj.PrintFunction();
    for (auto& field : dataFields)
    {
        outFile << field << ",";
    }
    outFile << std::endl;  // end line
}

template <typename V>
void HistoricalDataConnector<V>::Subscribe(std::ifstream& /*fileStream*/)
{
    // Not implemented
}

// ============================================================================
// CLASS: HistoricalDataListener<V>
// ============================================================================
/**
 * HistoricalDataListener listens for add/remove/update events
 * from other services and persists them.
 */
template <typename V>
class HistoricalDataListener : public ServiceListener<V>
{
public:
    // Constructor
    explicit HistoricalDataListener(HistoricalDataService<V>* svcPtr);

    // Destructor (no override, since base has no virtual dtor)
    ~HistoricalDataListener();

    // Listener callbacks
    void ProcessAdd(V& dataObj) override;
    void ProcessRemove(V& dataObj) override;
    void ProcessUpdate(V& dataObj) override;

private:
    HistoricalDataService<V>* parentService;
};

// --------------------------------------------------------------------------
// Implementation of HistoricalDataListener<V>
// --------------------------------------------------------------------------
template <typename V>
HistoricalDataListener<V>::HistoricalDataListener(HistoricalDataService<V>* svcPtr)
    : parentService(svcPtr)
{
}

template <typename V>
HistoricalDataListener<V>::~HistoricalDataListener()
{
    // default destructor
}

template <typename V>
void HistoricalDataListener<V>::ProcessAdd(V& dataObj)
{
    std::string persistKey = dataObj.GetProduct().GetProductId();
    parentService->PersistData(persistKey, dataObj);
}

template <typename V>
void HistoricalDataListener<V>::ProcessRemove(V& /*dataObj*/)
{
    // Not used
}

template <typename V>
void HistoricalDataListener<V>::ProcessUpdate(V& /*dataObj*/)
{
    // Not used
}

#endif // HISTORICAL_DATA_SERVICE_HPP
