/**
 * inquiryservice.hpp
 * Defines the data types and Service for customer inquiries.
 * @author
 *   Breman Thuraisingham
 * @coauthor
 *   Zixiuji Wang
 */

#ifndef INQUIRY_SERVICE_HPP
#define INQUIRY_SERVICE_HPP

#include "soa.hpp"
#include "tradebookingservice.hpp"
#include "utility.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <map>
#include <iostream>

// ============================================================================
// ENUM: InquiryState
// ============================================================================
/**
 * Various states for an inquiry lifecycle.
 */
enum InquiryState
{
    RECEIVED,
    QUOTED,
    DONE,
    REJECTED,
    CUSTOMER_REJECTED
};

// ============================================================================
// CLASS: Inquiry<T>
// ============================================================================
/**
 * Models a customer inquiry for a given product, side, and quantity.
 * Type T is the product type.
 */
template <typename T>
class Inquiry
{
public:
    // Constructors
    Inquiry() = default;
    Inquiry(std::string _inquiryId, const T& _product, Side _side, long _quantity,
            double _price, InquiryState _state);

    // Accessors
    const std::string& GetInquiryId() const;
    const T& GetProduct() const;
    Side GetSide() const;
    long GetQuantity() const;
    double GetPrice() const;
    InquiryState GetState() const;

    // Mutators
    void SetPrice(double _price);
    void SetState(InquiryState _state);

    // Print function for logging or display
    std::vector<std::string> PrintFunction() const;

private:
    std::string inquiryId;
    T product;
    Side side;
    long quantity;
    double price;
    InquiryState state;
};

// -------------------- Implementation of Inquiry<T> --------------------
template <typename T>
Inquiry<T>::Inquiry(std::string _inquiryId, const T& _product, Side _side,
                    long _quantity, double _price, InquiryState _state)
    : inquiryId(_inquiryId), product(_product), side(_side),
      quantity(_quantity), price(_price), state(_state)
{
}

template <typename T>
const std::string& Inquiry<T>::GetInquiryId() const
{
    return inquiryId;
}

template <typename T>
const T& Inquiry<T>::GetProduct() const
{
    return product;
}

template <typename T>
Side Inquiry<T>::GetSide() const
{
    return side;
}

template <typename T>
long Inquiry<T>::GetQuantity() const
{
    return quantity;
}

template <typename T>
double Inquiry<T>::GetPrice() const
{
    return price;
}

template <typename T>
InquiryState Inquiry<T>::GetState() const
{
    return state;
}

template <typename T>
void Inquiry<T>::SetPrice(double _price)
{
    price = _price;
}

template <typename T>
void Inquiry<T>::SetState(InquiryState _state)
{
    state = _state;
}

template <typename T>
std::vector<std::string> Inquiry<T>::PrintFunction() const
{
    // Convert members to strings
    std::string inqIdStr = inquiryId;
    std::string prodIdStr = product.GetProductId();
    std::string sideStr = (side == BUY) ? "BUY" : "SELL";
    std::string qtyStr = std::to_string(quantity);
    std::string priceStr = price2string(price);

    std::string stateStr;
    switch (state)
    {
    case RECEIVED:
        stateStr = "RECEIVED";
        break;
    case QUOTED:
        stateStr = "QUOTED";
        break;
    case DONE:
        stateStr = "DONE";
        break;
    case REJECTED:
        stateStr = "REJECTED";
        break;
    case CUSTOMER_REJECTED:
        stateStr = "CUSTOMER_REJECTED";
        break;
    }

    return { inqIdStr, prodIdStr, sideStr, qtyStr, priceStr, stateStr };
}

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
template <typename T>
class InquiryConnector;

// ============================================================================
// CLASS: InquiryService<T>
// ============================================================================
/**
 * InquiryService manages customer inquiries keyed by an inquiry ID (string).
 * Type T is the product type.
 */
template <typename T>
class InquiryService : public Service<std::string, Inquiry<T>>
{
public:
    // Constructor
    InquiryService();

    // Retrieve an Inquiry<T> by ID
    Inquiry<T>& GetData(std::string _key) override;

    // Connector callback for new/updated inquiries
    void OnMessage(Inquiry<T>& _data) override;

    // Add a listener for Inquiry<T> events
    void AddListener(ServiceListener<Inquiry<T>>* _listener) override;

    // Retrieve all listeners
    const std::vector<ServiceListener<Inquiry<T>>*>& GetListeners() const override;

    // Access the connector
    InquiryConnector<T>* GetConnector();

    // Send a quote back to the client
    void SendQuote(const std::string& _inquiryId, double _price);

    // Reject an inquiry from the client
    void RejectInquiry(const std::string& _inquiryId);

private:
    std::map<std::string, Inquiry<T>> inquiries;
    std::vector<ServiceListener<Inquiry<T>>*> listeners;
    InquiryConnector<T>* connector;
};

// -------------------- Implementation of InquiryService<T> --------------------
template <typename T>
InquiryService<T>::InquiryService()
    : inquiries(), listeners(), connector(nullptr)
{
    inquiries = std::map<std::string, Inquiry<T>>();
    listeners = std::vector<ServiceListener<Inquiry<T>>*>();
    connector = new InquiryConnector<T>(this);
}

template <typename T>
Inquiry<T>& InquiryService<T>::GetData(std::string _key)
{
    return inquiries[_key];
}

template <typename T>
void InquiryService<T>::OnMessage(Inquiry<T>& _data)
{
    InquiryState currentState = _data.GetState();
    if (currentState == RECEIVED)
    {
        // Store inquiry if it's newly received
        inquiries[_data.GetInquiryId()] = _data;

        // Publish to connector (this may change the state to QUOTED)
        connector->Publish(_data);
    }

    // If the state is QUOTED, we move to DONE and notify listeners
    if (currentState == QUOTED)
    {
        _data.SetState(DONE);
        inquiries[_data.GetInquiryId()] = _data;

        // Notify all service listeners
        for (auto& ls : listeners)
        {
            ls->ProcessAdd(_data);
        }
    }
}

template <typename T>
void InquiryService<T>::AddListener(ServiceListener<Inquiry<T>>* _listener)
{
    listeners.push_back(_listener);
}

template <typename T>
const std::vector<ServiceListener<Inquiry<T>>*>& InquiryService<T>::GetListeners() const
{
    return listeners;
}

template <typename T>
InquiryConnector<T>* InquiryService<T>::GetConnector()
{
    return connector;
}

template <typename T>
void InquiryService<T>::SendQuote(const std::string& _inquiryId, double _price)
{
    // Update the price for the given inquiry
    Inquiry<T>& targetInquiry = inquiries[_inquiryId];
    targetInquiry.SetPrice(_price);

    // Notify all listeners that something changed
    for (auto& ls : listeners)
    {
        ls->ProcessAdd(targetInquiry);
    }
}

template <typename T>
void InquiryService<T>::RejectInquiry(const std::string& _inquiryId)
{
    // Mark the inquiry as REJECTED
    Inquiry<T>& targetInquiry = inquiries[_inquiryId];
    targetInquiry.SetState(REJECTED);
}

// ============================================================================
// CLASS: InquiryConnector<T>
// ============================================================================
/**
 * InquiryConnector is responsible for subscribing to and publishing
 * Inquiry<T> data to/from the InquiryService.
 * Type T is the product type.
 */
template <typename T>
class InquiryConnector : public Connector<Inquiry<T>>
{
public:
    // Constructor
    InquiryConnector(InquiryService<T>* _service);

    // Publish data (may change state from RECEIVED to QUOTED)
    void Publish(Inquiry<T>& _data) override;

    // Subscribe from a file stream (e.g. "inquiries.txt")
    void Subscribe(std::ifstream& _data) override;

    // Overloaded Subscribe to handle a single Inquiry update
    void Subscribe(Inquiry<T>& _data);

private:
    InquiryService<T>* inquiryService;
};

// -------------------- Implementation of InquiryConnector<T> --------------------
template <typename T>
InquiryConnector<T>::InquiryConnector(InquiryService<T>* _service)
    : inquiryService(_service)
{
}

template <typename T>
void InquiryConnector<T>::Publish(Inquiry<T>& _data)
{
    // If the Inquiry is just RECEIVED, move it to QUOTED
    if (_data.GetState() == RECEIVED)
    {
        _data.SetState(QUOTED);
        // Re-subscribe to process state change
        this->Subscribe(_data);
    }
}

template <typename T>
void InquiryConnector<T>::Subscribe(std::ifstream& inputStream)
{
    // Read lines (e.g. from "inquiries.txt") and create Inquiry objects
    std::string lineStr;
    while (std::getline(inputStream, lineStr))
    {
        if (lineStr.empty()) continue;

        std::stringstream ss(lineStr);
        std::string token;
        std::vector<std::string> fields;
        while (std::getline(ss, token, ','))
        {
            fields.push_back(token);
        }

        if (fields.size() < 6) continue; // Expect 6 fields: ID, productId, side, qty, price, state

        // Parse fields
        std::string inquiryIdStr = fields[0];
        std::string productIdStr = fields[1];
        Side inquirySide = (fields[2] == "BUY") ? BUY : SELL;
        long parsedQty = stol(fields[3]);
        double parsedPrice = string2price(fields[4]);

        InquiryState parsedState;
        if (fields[5] == "RECEIVED")
            parsedState = RECEIVED;
        else if (fields[5] == "QUOTED")
            parsedState = QUOTED;
        else if (fields[5] == "DONE")
            parsedState = DONE;
        else if (fields[5] == "REJECTED")
            parsedState = REJECTED;
        else
            parsedState = CUSTOMER_REJECTED;

        // Convert product ID to actual product (e.g., Bond)
        T productObj = GetBond(productIdStr);

        // Create an Inquiry object
        Inquiry<T> newInquiry(inquiryIdStr, productObj, inquirySide,
                              parsedQty, parsedPrice, parsedState);

        // Notify the service
        inquiryService->OnMessage(newInquiry);
    }
}

template <typename T>
void InquiryConnector<T>::Subscribe(Inquiry<T>& _data)
{
    // Forward the updated inquiry to the service
    inquiryService->OnMessage(_data);
}

#endif // INQUIRY_SERVICE_HPP
