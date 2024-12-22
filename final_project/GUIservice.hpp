/**
 * GUIservice.hpp
 * Defines data types, the GUIService and GUIListener related to GUI.
 *
 * @author Zixiuji Wang
 */
#ifndef GUI_SERVICE_HPP
#define GUI_SERVICE_HPP

#include <chrono>
#include <fstream>
#include <string>
#include <thread>
#include "soa.hpp"
#include "pricingservice.hpp"
#include "utility.hpp"

// Throttle value in milliseconds
constexpr int THROTTLE_MILLISECONDS = 300;

// Forward declarations
template<typename T>
class GUIConnector;

template<typename T>
class GUIListener;

// GUIService class definition
template<typename T>
class GUIService : Service<string, Price<T>>  {

public:
    // Constructor
    GUIService();

    // Destructor
    ~GUIService();

    // Get data from the service given a key
    Price<T>& GetData(string _key);

    // The callback that a Connector should invoke for any new or updated data
    void OnMessage(Price<T>& _data);

    // Add a listener to the service
    void AddListener(ServiceListener<Price<T>>* listener);

    // Get all listeners on the service
    const vector<ServiceListener<Price<T>>*>& GetListeners() const;

    // Get the connector
    GUIConnector<T>* GetConnector();

    // Get the listener
    ServiceListener<Price<T>>* GetListener();

    // Get the current time
    int GetTime() const;

    // Set the time
    void SetTime(int _time);

private:
    map<string, Price<T>> GUIs;
    vector<ServiceListener<Price<T>>*> listeners;
    GUIConnector<T>* connector;
    ServiceListener<Price<T>>* listener;
    int timeCount;
};

template<typename T>
GUIService<T>::GUIService() {
    GUIs = map<string, Price<T>>();
    listeners = vector<ServiceListener<Price<T>>*>();
    connector = new GUIConnector<T>(this);
    listener = new GUIListener<T>(this);
    timeCount = 0;
}

template<typename T>
GUIService<T>::~GUIService() {}

template<typename T>
Price<T>& GUIService<T>::GetData(string _key) {
    return GUIs[_key];
}

template<typename T>
void GUIService<T>::OnMessage(Price<T>& _data) {
    string product_id = _data.GetProduct().GetProductId();
    GUIs[product_id] = _data;
    connector->Publish(_data);
}

template<typename T>
void GUIService<T>::AddListener(ServiceListener<Price<T>>* _listener)
{
    listeners.push_back(_listener);
}

template<typename T>
const vector<ServiceListener<Price<T>>*>& GUIService<T>::GetListeners() const
{
    return listeners;
}

template<typename T>
GUIConnector<T>* GUIService<T>::GetConnector()
{
    return connector;
}

template<typename T>
ServiceListener<Price<T>>* GUIService<T>::GetListener()
{
    return listener;
}

template<typename T>
int GUIService<T>::GetTime() const
{
    return timeCount;
}

template<typename T>
void GUIService<T>::SetTime(int _time)
{
    timeCount = _time;
}

// GUIConnector class definition
template<typename T>
class GUIConnector : public Connector<Price<T>> {
public:
    GUIConnector(GUIService<T>* service);

    void Publish(Price<T>& _data);

    // Not implemented
    void Subscribe(ifstream& _data);

private:
    GUIService<T>* guiService;
};

template<typename T>
GUIConnector<T>::GUIConnector(GUIService<T>* service) : guiService(service) {}

template<typename T>
void GUIConnector<T>::Publish(Price<T>& _data)
{
    int _time = guiService->GetTime();

    auto timePoint = chrono::system_clock::now();
    auto sec = chrono::time_point_cast<chrono::seconds>(timePoint);
    auto millisec = chrono::duration_cast<chrono::milliseconds>(timePoint - sec);
    long currentTime = static_cast<long>(millisec.count());

    // Update time
    while (currentTime < _time) {
        currentTime += 1000;
    }
    if (currentTime - _time >= THROTTLE_MILLISECONDS)
    {
        guiService->SetTime(currentTime);
        ofstream _file;
        _file.open("Data/Output/gui.txt", ios::app);

        if (!_file.is_open()) {
            cerr << "Error: Unable to open file at " << "Data/Output/gui.txt" << endl;
            return;
        }

        auto now = microsec_clock::local_time();
        _file << now << ",";
        for (auto& s : _data.PrintFunction())
        {
            _file << s << ",";
        }
        _file << "\n";
    }
}

template<typename T>
void GUIConnector<T>::Subscribe(ifstream& _data) {}

// GUIListener class definition
template<typename T>
class GUIListener : public ServiceListener<Price<T>> {
public:
    GUIListener(GUIService<T>* guiService);

    // Listener callback to process an add event to the Service
    void ProcessAdd(Price<T>& _data);

    // Listener callback to process a remove event to the Service
    void ProcessRemove(Price<T>& _data);

    // Listener callback to process an update event to the Service
    void ProcessUpdate(Price<T>& _data);

private:
    GUIService<T>* guiService;
};

template<typename T>
GUIListener<T>::GUIListener(GUIService<T>* service) : guiService(service) {}

template<typename T>
void GUIListener<T>::ProcessAdd(Price<T>& _data)
{
    guiService->OnMessage(_data);
}

// Do nothing for remove and update events
template<typename T>
void GUIListener<T>::ProcessRemove(Price<T>& _data) {}

template<typename T>
void GUIListener<T>::ProcessUpdate(Price<T>& _data) {}

#endif // GUI_SERVICE_HPP
