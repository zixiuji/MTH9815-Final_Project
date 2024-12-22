/**
 * riskservice.hpp
 * Defines the data types and Service for fixed income risk.
 *
 * @author Breman Thuraisingham
 * @coauthor Zixiuji Wang
 */
#ifndef RISK_SERVICE_HPP
#define RISK_SERVICE_HPP

#include "positionservice.hpp"
#include "soa.hpp"

// Reasonable PV01 values of the bonds
// from the internet
const map<string, double> bondPV01({{
  {"912828V23", 0.019},
  {"912828W22", 0.028},
  {"912828X21", 0.046},
  {"912828Y20", 0.064},
  {"912828Z19", 0.091},
  {"912810FZ8", 0.142},
  {"912810GZ6", 0.183}
}

});

/**
 * PV01 risk.
 * Type T is the product type.
 */
template <typename T> class PV01 {

  public:
    // ctor for a PV01 value
    PV01() = default;
    PV01(const T& _product, double _pv01, long _quantity);

    // Get the product on this PV01 value
    const T& GetProduct() const;

    // Get the PV01 value
    double GetPV01() const;

    // Get the quantity that this risk value is associated with
    long GetQuantity() const;

    // Set the quantity that this risk value is associated with
    void SetQuantity(long _q);

    vector<string> PrintFunction() const;

  private:
    T product;
    double pv01;
    long quantity;
};

template <typename T>
PV01<T>::PV01(const T& _product, double _pv01, long _quantity)
    : product(_product) {
    pv01 = _pv01;
    quantity = _quantity;
}

template <typename T> const T& PV01<T>::GetProduct() const { return product; }

template <typename T> double PV01<T>::GetPV01() const { return pv01; }

template <typename T> long PV01<T>::GetQuantity() const { return quantity; }

template <typename T> void PV01<T>::SetQuantity(long _q) { quantity = _q; }

template <typename T> vector<string> PV01<T>::PrintFunction() const {
    string _product = product.GetProductId();
    string _pv01 = to_string(pv01);
    string _quantity = to_string(quantity);

    vector<string> _strings{_product, _pv01, _quantity};
    return _strings;
}

/**
 * A bucket sector to bucket a group of securities.
 * We can then aggregate bucketed risk to this bucket.
 * Type T is the product type.
 */
template <typename T> class BucketedSector {

  public:
    // ctor for a bucket sector
    BucketedSector(const vector<T>& _products, string _name);

    // Get the products associated with this bucket
    const vector<T>& GetProducts() const;

    // Get the name of the bucket
    const string& GetName() const;

  private:
    vector<T> products;
    string name;
};

template <typename T>
BucketedSector<T>::BucketedSector(const vector<T>& _products, string _name)
    : products(_products) {
    name = _name;
}

template <typename T> const vector<T>& BucketedSector<T>::GetProducts() const {
    return products;
}

template <typename T> const string& BucketedSector<T>::GetName() const {
    return name;
}

// Pre-declearations to avoid errors
template <typename T> class RiskServiceListener;

/**
 * Risk Service to vend out risk for a particular security and across a risk
 * bucketed sector. Keyed on product identifier. Type T is the product type.
 */
template <typename T> class RiskService : public Service<string, PV01<T>> {
  public:
    // ctor
    RiskService();

    // Add a position that the service will risk
    void AddPosition(Position<T>& position);

    // Get the bucketed risk for the bucket sector
    const PV01<BucketedSector<T>>&
    GetBucketedRisk(const BucketedSector<T>& sector) const;

    // Get data from the given key
    PV01<T>& GetData(string _key);

    // Call back function receiving new data
    void OnMessage(PV01<T>& _data);

    // Add a listener to the Service
    void AddListener(ServiceListener<PV01<T>>* _listener);

    // Get all listeners on the Service
    const vector<ServiceListener<PV01<T>>*>& GetListeners() const;

    // Get the BondPositionService listener of the service
    RiskServiceListener<T>* GetListener();

  private:
    map<string, PV01<T>> pv01s;
    vector<ServiceListener<PV01<T>>*> listeners;
    RiskServiceListener<T>* listener;
};

template <typename T> RiskService<T>::RiskService() {
    pv01s = map<string, PV01<T>>();
    listeners = vector<ServiceListener<PV01<T>>*>();
    listener = new RiskServiceListener<T>(this);
}

template <typename T> PV01<T>& RiskService<T>::GetData(string _key) {
    return pv01s[_key];
}

template <typename T> void RiskService<T>::OnMessage(PV01<T>& _data) {
    pv01s[_data.GetProduct().GetProductId()] = _data;
}

template <typename T>
void RiskService<T>::AddListener(ServiceListener<PV01<T>>* _listener) {
    listeners.push_back(_listener);
}

template <typename T>
const vector<ServiceListener<PV01<T>>*>& RiskService<T>::GetListeners() const {
    return listeners;
}

template <typename T> RiskServiceListener<T>* RiskService<T>::GetListener() {
    return listener;
}

template <typename T> void RiskService<T>::AddPosition(Position<T>& _position) {
    T _product = _position.GetProduct();
    string _id = _product.GetProductId();
    double _pv01Value = bondPV01.at(_id);
    long _quantity = _position.GetAggregatePosition();
    PV01<T> _pv01(_product, _pv01Value, _quantity);
    pv01s[_id] = _pv01;

    for (auto& l : listeners) {
        l->ProcessAdd(_pv01);
    }
}

template <typename T>
const PV01<BucketedSector<T>>&
RiskService<T>::GetBucketedRisk(const BucketedSector<T>& _sector) const {
    BucketedSector<T> _product = _sector;
    double _pv01 = 0.0;
    long _quantity = 1;

    vector<T>& _products = _sector.GetProducts();

    for (auto& p : _products) {
        string _pId = p.GetProductId();
        long _q = pv01s[_pId].GetQuantity();
        double _val = pv01s[_pId].GetPV01();
        _pv01 += _val * (double)_q;
    }

    return PV01<BucketedSector<T>>(_product, _pv01, _quantity);
}

/**
 * Risk Service Listener
 * subscribe data from BondPositionService  to BondRiskService.
 * Type T is the product type.
 */
template <typename T>
class RiskServiceListener : public ServiceListener<Position<T>> {

  private:
    RiskService<T>* service;

  public:
    // Ctor
    RiskServiceListener(RiskService<T>* _service);

    // Listener callback to process an add event to the Service
    void ProcessAdd(Position<T>& _data);

    // Listener callback to process a remove event to the Service
    void ProcessRemove(Position<T>& _data);

    // Listener callback to process an update event to the Service
    void ProcessUpdate(Position<T>& _data);
};

template <typename T>
RiskServiceListener<T>::RiskServiceListener(RiskService<T>* _service) {
    service = _service;
}

template <typename T>
void RiskServiceListener<T>::ProcessAdd(Position<T>& _data) {
    service->AddPosition(_data);
}

template <typename T>
void RiskServiceListener<T>::ProcessRemove(Position<T>& _data) {}

template <typename T>
void RiskServiceListener<T>::ProcessUpdate(Position<T>& _data) {}

#endif