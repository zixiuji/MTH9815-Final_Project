/**
 * executionservice.hpp
 * Defines the Service and Listener for executions.
 *
 * Author: Breman Thuraisingham
 * Coauthor: Zixiuji Wang
 */
#ifndef EXECUTION_SERVICE_HPP
#define EXECUTION_SERVICE_HPP

#include <string>
#include "soa.hpp"
#include "execution.hpp"
#include "marketdataservice.hpp"

// Forward declarations to avoid errors.
template<typename T>
class ExecutionServiceListener;

/**
 * ExecutionService
 * A service that executes orders on an exchange, keyed by product ID.
 * Type T is the product type (e.g., Bond).
 */
template<typename T>
class ExecutionService : public Service<std::string, ExecutionOrder<T>>
{
public:
    // Constructor
    ExecutionService();

    // Retrieve an ExecutionOrder by product ID
    ExecutionOrder<T>& GetData(std::string id) override;

    // Connector/Listener callback for new or updated data
    void OnMessage(ExecutionOrder<T>& dataObj) override;

    // Add a listener for ExecutionOrder<T> events
    void AddListener(ServiceListener<ExecutionOrder<T>>* listenerPtr) override;

    // Return all registered listeners
    const std::vector<ServiceListener<ExecutionOrder<T>>*>& GetListeners() const override;

    // Retrieve the specialized listener
    ExecutionServiceListener<T>* GetListener();

    // Execute an order upon receiving a request
    void ExecuteOrder(ExecutionOrder<T>& execOrder);

private:
    std::map<std::string, ExecutionOrder<T>> executionOrders;
    std::vector<ServiceListener<ExecutionOrder<T>>*> listeners;
    ExecutionServiceListener<T>* listener;
};

// -------------------- Implementation of ExecutionService<T> --------------------

template<typename T>
ExecutionService<T>::ExecutionService()
{
    executionOrders = std::map<std::string, ExecutionOrder<T>>();
    listeners       = std::vector<ServiceListener<ExecutionOrder<T>>*>();
    listener        = new ExecutionServiceListener<T>(this);
}

template<typename T>
ExecutionOrder<T>& ExecutionService<T>::GetData(std::string id)
{
    return executionOrders[id];
}

template<typename T>
void ExecutionService<T>::OnMessage(ExecutionOrder<T>& dataObj)
{
    std::string prodId = dataObj.GetProduct().GetProductId();
    executionOrders[prodId] = dataObj;
}

template<typename T>
void ExecutionService<T>::AddListener(ServiceListener<ExecutionOrder<T>>* listenerPtr)
{
    listeners.push_back(listenerPtr);
}

template<typename T>
const std::vector<ServiceListener<ExecutionOrder<T>>*>& ExecutionService<T>::GetListeners() const
{
    return listeners;
}

template<typename T>
ExecutionServiceListener<T>* ExecutionService<T>::GetListener()
{
    return listener;
}

template<typename T>
void ExecutionService<T>::ExecuteOrder(ExecutionOrder<T>& execOrder)
{
    std::string prodId = execOrder.GetProduct().GetProductId();
    executionOrders[prodId] = execOrder;

    // Notify all listeners
    for (auto& ls : listeners)
    {
        ls->ProcessAdd(execOrder);
    }
}

/**
 * ExecutionServiceListener
 * Subscribes to AlgoExecution<T> data from the BondAlgoExecutionService.
 * Type T is the product type.
 */
template<typename T>
class ExecutionServiceListener : public ServiceListener<AlgoExecution<T>>
{
public:
    // Constructor
    explicit ExecutionServiceListener(ExecutionService<T>* svc);

    // Listener callbacks
    void ProcessAdd(AlgoExecution<T>& algoExecObj) override;
    void ProcessRemove(AlgoExecution<T>& algoExecObj) override;
    void ProcessUpdate(AlgoExecution<T>& algoExecObj) override;

private:
    ExecutionService<T>* service;
};

// -------------------- Implementation of ExecutionServiceListener<T> --------------------

template<typename T>
ExecutionServiceListener<T>::ExecutionServiceListener(ExecutionService<T>* svc)
    : service(svc)
{
}

template<typename T>
void ExecutionServiceListener<T>::ProcessAdd(AlgoExecution<T>& algoExecObj)
{
    // Retrieve the ExecutionOrder from AlgoExecution
    ExecutionOrder<T>* execOrderPtr = algoExecObj.GetExecutionOrder();

    // First store the order
    service->OnMessage(*execOrderPtr);

    // Then execute it
    service->ExecuteOrder(*execOrderPtr);
}

template<typename T>
void ExecutionServiceListener<T>::ProcessRemove(AlgoExecution<T>& /*algoExecObj*/)
{
    // Not implemented
}

template<typename T>
void ExecutionServiceListener<T>::ProcessUpdate(AlgoExecution<T>& /*algoExecObj*/)
{
    // Not implemented
}

#endif // EXECUTION_SERVICE_HPP
