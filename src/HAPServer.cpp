#include "HomeKitAccessory.h"
#include <cstring>

HAPServer HKAccessory;

void HAPServer::_onRequestReceived(hap_network_connection * conn) {
    auto req = new HAPUserHelper(conn);

    delete req;
}

void HAPServer::_s_onRequestReceived(hap_network_connection *conn, void *arg) {
    static_cast<HAPServer *>(arg)->_onRequestReceived(conn);
}

void HAPServer::begin(uint16_t port) {
    _clearEventListeners();
    _clearEventQueue();

    _onSelf(HAPEvent::HAP_SD_NEEDED_UPDATE, &HAPServer::_updateSDRecords);

    server_conn = new hap_network_connection;
    server_conn->raw = nullptr;
    server_conn->user = nullptr;
    server_conn->request_cb_arg = this;
    server_conn->request_cb = &HAPServer::_s_onRequestReceived;

    hap_network_init_bind(server_conn, port);
    mdns_handle = hap_service_discovery_init(deviceName, port);
    emit(HAPEvent::HAP_SD_NEEDED_UPDATE);
}

void HAPServer::handle() {
    //networks
    hap_network_loop();

    //handle events
    auto currentEvent = _dequeueEvent();
    if(currentEvent){
        auto currentListener = eventListeners;
        while (currentListener != nullptr){
            if(currentListener->listening == currentEvent->name){
                if(currentListener->onEvent) currentListener->onEvent(currentEvent);
                if(currentListener->_internalOnEvent) (this->*(currentListener->_internalOnEvent))(currentEvent);
            }
            currentListener = currentListener->next;
        }
        if(currentEvent->didEmit) currentEvent->didEmit(currentEvent);
        delete currentEvent;
    }
}

void HAPServer::_clearEventQueue() {
    while (auto current = _dequeueEvent()){
        if(current->didEmit) current->didEmit(current);
        delete current;
    }
}

HAPEvent *HAPServer::_dequeueEvent() {
    auto current = eventQueue;
    if(current) eventQueue = current->next;
    return current;
}

HAPEventListener * HAPServer::on(HAPEvent::EventID id, HAPEventListener::Callback cb) {
    auto listener = new HAPEventListener();
    listener->onEvent = cb;
    listener->listening = id;
    listener->next = nullptr;

    HAPEventListener ** appendListener = &eventListeners;
    while ((*appendListener) != nullptr) appendListener = &((*appendListener)->next);
    *appendListener = listener;

    return listener;
}

void HAPServer::emit(HAPEvent::EventID name, void *args, HAPEventListener::Callback onCompletion) {
    auto event = new HAPEvent();
    event->name = name;
    event->argument = args;
    event->didEmit = onCompletion;

    HAPEvent ** appendEvent = &eventQueue;
    while ((*appendEvent) != nullptr) appendEvent = &((*appendEvent)->next);
    *appendEvent = event;
}

HAPEventListener * HAPServer::_onSelf(HAPEvent::EventID name, HAPEventListener::HAPCallback cb) {
    auto listener = on(name, nullptr);
    listener->_internalOnEvent = cb;
    return listener;
}

void HAPServer::_clearEventListeners() {
    auto current = eventListeners;
    while (current != nullptr){
        auto next = current->next;
        delete current;
        current = next;
    }
    eventListeners = nullptr;
}

void HAPServer::_updateSDRecords(HAPEvent *) {
    hap_sd_txt_item records[] = {
            { "c#", "2" },
            { "ff", "0" },
            { "id", deviceId },
            { "md", modelName },
            { "pv", "1.0" },
            { "s#", "1" },
            { "sf", "1" },
            { "ci", "2" },
            { nullptr, nullptr }
    };
    hap_service_discovery_update(mdns_handle, records);
}

HAPServer::~HAPServer() {

}
