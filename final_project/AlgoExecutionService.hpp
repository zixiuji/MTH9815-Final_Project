/**
 * AlgoExecutionService.hpp
 * Defines the data types and Service for algo executions.
 *
 * Author: Zixiuji Wang
 */

#ifndef ALGO_EXECUTION_SERVICE_HPP
#define ALGO_EXECUTION_SERVICE_HPP

#include "execution.hpp"
#include "marketdataservice.hpp"

using namespace std;

/**
 * AlgoExecution
 * Encapsulates an ExecutionOrder<T> for algorithmic trading decisions.
 * Type T is the product type.
 */
template <typename T>
class AlgoExecution
{
public:
    // Constructors
    AlgoExecution() = default;
    AlgoExecution(const T& prod, PricingSide ps, std::string oid,
                  OrderType ot, double pr, long visQty, long hidQty,
                  std::string parentOid, bool childFlag);

    // Retrieve the associated ExecutionOrder
    ExecutionOrder<T>* GetExecutionOrder() const;

private:
    ExecutionOrder<T>* executionOrder;
};

// -------------------- Implementation of AlgoExecution<T> --------------------

template <typename T>
AlgoExecution<T>::AlgoExecution(const T& prod, PricingSide ps,
                                std::string oid, OrderType ot,
                                double pr, long visQty, long hidQty,
                                std::string parentOid, bool childFlag)
{
    executionOrder = new ExecutionOrder<T>(
        prod, ps, oid, ot, pr, visQty, hidQty, parentOid, childFlag
    );
}

template <typename T>
ExecutionOrder<T>* AlgoExecution<T>::GetExecutionOrder() const
{
    return executionOrder;
}

// ---------------------------------------------------------------------------
// Forward declarations
template <typename T>
class AlgoExecutionServiceListener;

/**
 * AlgoExecutionService
 * Manages AlgoExecution objects keyed by product ID.
 * Type T is the product type.
 */
template <typename T>
class AlgoExecutionService : public Service<std::string, AlgoExecution<T>>
{
public:
    // Constructor / Destructor
    AlgoExecutionService();
    ~AlgoExecutionService();

    // Retrieve AlgoExecution<T> by product ID
    AlgoExecution<T>& GetData(std::string key) override;

    // Connector/Listener callback
    void OnMessage(AlgoExecution<T>& dataObj) override;

    // Add a service listener
    void AddListener(ServiceListener<AlgoExecution<T>>* listenerPtr) override;

    // Retrieve all service listeners
    const std::vector<ServiceListener<AlgoExecution<T>>*>& GetListeners() const override;

    // Retrieve the specialized listener
    AlgoExecutionServiceListener<T>* GetListener();

    // Perform an algo-based trade given an OrderBook
    void AlgoExecutionTrade(OrderBook<T>& orderBookObj);

private:
    std::map<std::string, AlgoExecution<T>> algoExecutions;
    std::vector<ServiceListener<AlgoExecution<T>>*> listeners;
    AlgoExecutionServiceListener<T>* listener;
    long executionCount;
};

// -------------------- Implementation of AlgoExecutionService<T> --------------------

template <typename T>
AlgoExecutionService<T>::AlgoExecutionService()
{
    algoExecutions = std::map<std::string, AlgoExecution<T>>();
    listeners      = std::vector<ServiceListener<AlgoExecution<T>>*>();
    listener       = new AlgoExecutionServiceListener<T>(this);
    executionCount = 0;
}

template <typename T>
AlgoExecutionService<T>::~AlgoExecutionService()
{
    // Default destructor
}

template <typename T>
AlgoExecution<T>& AlgoExecutionService<T>::GetData(std::string key)
{
    return algoExecutions[key];
}

template <typename T>
void AlgoExecutionService<T>::OnMessage(AlgoExecution<T>& dataObj)
{
    // Store or update the algo execution keyed by the product's ID
    std::string prodId = dataObj.GetExecutionOrder()->GetProduct().GetProductId();
    algoExecutions[prodId] = dataObj;
}

template <typename T>
void AlgoExecutionService<T>::AddListener(ServiceListener<AlgoExecution<T>>* listenerPtr)
{
    listeners.push_back(listenerPtr);
}

template <typename T>
const std::vector<ServiceListener<AlgoExecution<T>>*>&
AlgoExecutionService<T>::GetListeners() const
{
    return listeners;
}

template <typename T>
AlgoExecutionServiceListener<T>* AlgoExecutionService<T>::GetListener()
{
    return listener;
}

/**
 * AlgoExecutionTrade
 * Only trades if the spread <= 1/128 to reduce cost of crossing the spread.
 * Chooses bid or offer side based on executionCount parity, then updates algoExec.
 */
template <typename T>
void AlgoExecutionService<T>::AlgoExecutionTrade(OrderBook<T>& orderBookObj)
{
    // Retrieve product ID
    std::string prodId = orderBookObj.GetProduct().GetProductId();

    // Extract best bid and offer
    Order bestBid   = orderBookObj.GetBidOffer().GetBidOrder();
    Order bestOffer = orderBookObj.GetBidOffer().GetOfferOrder();

    double bidPrice   = bestBid.GetPrice();
    long   bidQty     = bestBid.GetQuantity();
    double offerPrice = bestOffer.GetPrice();
    long   offerQty   = bestOffer.GetQuantity();

    // Evaluate if spread <= 1/128
    if ((offerPrice - bidPrice) <= 1.0 / 128.0)
    {
        double chosenPrice;
        long chosenQty;
        PricingSide chosenSide;

        if (executionCount % 2 == 1)
        {
            // Use bid side
            chosenPrice = bidPrice;
            chosenQty   = bidQty;
            chosenSide  = BID;
        }
        else
        {
            // Use offer side
            chosenPrice = offerPrice;
            chosenQty   = offerQty;
            chosenSide  = OFFER;
        }
        executionCount++;

        // Construct the AlgoExecution
        std::string oid = "AlgoExec" + std::to_string(executionCount);
        AlgoExecution<T> algoExec(
            orderBookObj.GetProduct(),
            chosenSide,
            oid,
            MARKET,
            chosenPrice,
            chosenQty,
            0,
            "PARENT_ORDER_ID",
            false
        );
        algoExecutions[prodId] = algoExec;

        // Notify all service listeners
        for (auto& ls : listeners)
        {
            ls->ProcessAdd(algoExec);
        }
    }
}

// ---------------------------------------------------------------------------
// CLASS: AlgoExecutionServiceListener<T>
/**
 * AlgoExecutionServiceListener
 * Receives OrderBook<T> from BondMarketDataService and triggers algo trades.
 */
template <typename T>
class AlgoExecutionServiceListener : public ServiceListener<OrderBook<T>>
{
public:
    // Constructor
    explicit AlgoExecutionServiceListener(AlgoExecutionService<T>* svc);

    // Listener callbacks
    void ProcessAdd(OrderBook<T>& bookData) override;
    void ProcessRemove(OrderBook<T>& bookData) override;
    void ProcessUpdate(OrderBook<T>& bookData) override;

private:
    AlgoExecutionService<T>* service;
};

// -------------------- Implementation of AlgoExecutionServiceListener<T> --------------------

template <typename T>
AlgoExecutionServiceListener<T>::AlgoExecutionServiceListener(AlgoExecutionService<T>* svc)
    : service(svc)
{
}

template <typename T>
void AlgoExecutionServiceListener<T>::ProcessAdd(OrderBook<T>& bookData)
{
    // Execute trade logic
    service->AlgoExecutionTrade(bookData);
}

template <typename T>
void AlgoExecutionServiceListener<T>::ProcessRemove(OrderBook<T>& /*bookData*/)
{
    // Not implemented
}

template <typename T>
void AlgoExecutionServiceListener<T>::ProcessUpdate(OrderBook<T>& /*bookData*/)
{
    // Not implemented
}

#endif // ALGO_EXECUTION_SERVICE_HPP
