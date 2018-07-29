#include "HomeKitAccessory.h"
#include "network.h"
#include "hap_crypto.h"

#include <cstring>

HAPServer HKAccessory;

void HAPServer::begin(uint16_t port) {
    _clearEventListeners();
    _clearEventQueue();

    delete pairingsManager;
    pairingsManager = new HAPPairingsManager(this);

    //Register all events handled internally by HAPServer
    _onSelf(HAPEvent::HAP_SD_NEEDED_UPDATE, &HAPServer::_updateSDRecords);
    _onSelf(HAPEvent::HAP_NET_RECEIVE_REQUEST, &HAPServer::_onRequestReceived);
    _onSelf(HAPEvent::HAP_NET_CONNECT, &HAPServer::_onConnect);
    _onSelf(HAPEvent::HAP_NET_DISCONNECT, &HAPServer::_onDisconnect);

    _onSelf(HAPEvent::HAPCRYPTO_DECRYPTED, &HAPServer::_onDataDecrypted);

    //For HAPPairingsManager
    _onSelf(HAPEvent::HAPCRYPTO_SRP_INIT_COMPLETE, &HAPServer::_onSetupInitComplete);
    _onSelf(HAPEvent::HAPCRYPTO_SRP_PROOF_COMPLETE, &HAPServer::_onSetupProofComplete);

    server_conn = new hap_network_connection;
    server_conn->raw = nullptr;
    server_conn->user = nullptr;
    server_conn->server = this;

    hap_network_init_bind(server_conn, port);
    mdns_handle = hap_service_discovery_init(deviceName, port);
    emit(HAPEvent::HAP_SD_NEEDED_UPDATE);
}

void HAPServer::handle() {
    //networks
    hap_network_loop();
    hap_service_discovery_loop(mdns_handle);

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

//TODO: tmp added for hexdump
#include <ctype.h>
void hexdump(const void *ptr, int buflen) {
    auto *buf = (unsigned const char*)ptr;
    int i, j;
    for (i=0; i<buflen; i+=16) {
        printf("%06x: ", i);
        for (j=0; j<16; j++)
            if (i+j < buflen)
                printf("%02x ", buf[i+j]);
            else
                printf("   ");
        printf(" ");
        for (j=0; j<16; j++)
            if (i+j < buflen)
                printf("%c", isprint(buf[i+j]) ? buf[i+j] : '.');
        printf("\n");
    }
}

void HAPServer::_onRequestReceived(HAPEvent * e) {
    auto req = new HAPUserHelper(e->arg<hap_network_connection>());
    req->retain();

    HAP_DEBUG("New Request to %d with method %d, length %u bytes", req->path(), req->method(), req->dataLength());
    hexdump(req->data(), req->dataLength());

    switch (req->path()){
        case PAIR_SETUP:
            pairingsManager->onPairSetup(req);
            break;
        default: HAP_DEBUG("Unimplemented path: %d", req->path());
    }

    req->release();
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
    hap_service_discovery_deinit(mdns_handle);
    mdns_handle = nullptr;
}

void HAPServer::_onConnect(HAPEvent * event) {
    auto c = event->arg<hap_network_connection>();
    c->user->pair_info = new hap_pair_info(this);
}

void HAPServer::_onDisconnect(HAPEvent * event) {
    auto user = event->arg<hap_user_connection>();
    delete user->pair_info;
    user->pair_info = nullptr;
}

void HAPServer::_onSetupInitComplete(HAPEvent * event) {
    pairingsManager->onPairSetupM2Finish(event->arg<hap_crypto_setup>());
}

void HAPServer::_onSetupProofComplete(HAPEvent * event) {
    pairingsManager->onPairSetupM4Finish(event->arg<hap_crypto_setup>());
}

void HAPServer::_onDataDecrypted(HAPEvent * event) {
    auto info = event->arg<hap_crypto_info>();
    auto pairInfo = info->session->pairInfo();

    //If isPairing or isVerifying, let PairingsManager handle this decryption
    if(pairInfo->isPairing){
        pairingsManager->onPairingDeviceDecryption(pairInfo, info->session);
    }
}
