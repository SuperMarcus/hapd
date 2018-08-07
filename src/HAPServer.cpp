#include "HomeKitAccessory.h"
#include "network.h"
#include "hap_crypto.h"
#include "HAPPersistingStorage.h"
#include "service/AccessoryInformation.h"

#include <cstring>

HAPServer HKAccessory;

void HAPServer::begin(uint16_t port) {
    _clearEventListeners();
    _clearEventQueue();
    _clearSubscribers();

    //Register all events handled internally by HAPServer
    _onSelf(HAPEvent::HAP_SD_NEEDED_UPDATE, &HAPServer::_updateSDRecords);
    _onSelf(HAPEvent::HAP_NET_RECEIVE_REQUEST, &HAPServer::_onRequestReceived);
    _onSelf(HAPEvent::HAP_NET_CONNECT, &HAPServer::_onConnect);
    _onSelf(HAPEvent::HAP_NET_DISCONNECT, &HAPServer::_onDisconnect);

    _onSelf(HAPEvent::HAPCRYPTO_DECRYPTED, &HAPServer::_onDataDecrypted);
    _onSelf(HAPEvent::HAPCRYPTO_ENCRYPTED, &HAPServer::_onDataEncrypted);
    _onSelf(HAPEvent::HAP_INITIALIZE_KEYPAIR, &HAPServer::_onInitKeypairReq);

    //For HAPPairingsManager
    _onSelf(HAPEvent::HAPCRYPTO_SRP_INIT_COMPLETE, &HAPServer::_onSetupInitComplete);
    _onSelf(HAPEvent::HAPCRYPTO_SRP_PROOF_COMPLETE, &HAPServer::_onSetupProofComplete);
    _onSelf(HAPEvent::HAP_DEVICE_PAIR, &HAPServer::_onDevicePair);
    _onSelf(HAPEvent::HAP_DEVICE_VERIFY, &HAPServer::_onDeviceVerify);

    delete storage;
    storage = new HAPPersistingStorage();

    delete pairingsManager;
    pairingsManager = new HAPPairingsManager(this);

    server_conn = new hap_network_connection;
    server_conn->raw = nullptr;
    server_conn->user = nullptr;
    server_conn->server = this;

    //Init the default accessory with aid 0
    //TODO: free all accessories
    delete accessories;
    accessories = new BaseAccessory(0, this);

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

    auto lineBuf = new char[256];
    for (i=0; i<buflen; i+=16) {
        memset(lineBuf, ' ', 255);
        lineBuf[255] = '\x00';
        auto _p = lineBuf;

        _p += sprintf(_p, "%06x: ", i);
        for (j=0; j<16; j++)
            if (i+j < buflen)
                _p += sprintf(_p, "%02x ", buf[i+j]);
            else
                _p += sprintf(_p, "   ");
        _p += sprintf(_p, " ");
        for (j=0; j<16; j++)
            if (i+j < buflen)
                _p += sprintf(_p, "%c", isprint(buf[i+j]) ? buf[i+j] : '.');
        HAP_DEBUG("%s", lineBuf);
    }
    delete[] lineBuf;
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
        case PAIR_VERIFY:
            pairingsManager->onPairVerify(req);
            break;
        case ACCESSORIES:
            if(req->method() == GET){ _sendAttributionDatabase(req); }
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

void HAPServer::_clearSubscribers() {
    auto current = subscribers;
    while (current != nullptr){
        auto next = current->next;
        delete current;
        current = next;
    }
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
    //TODO: free all accessories
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

void HAPServer:: _onDataDecrypted(HAPEvent * event) {
    auto info = event->arg<hap_crypto_info>();

    if((info->flags) & CRYPTO_FLAG_NETWORK){ // NOLINT
        if(hap_crypto_data_decrypt_did_succeed(info)){
            hap_http_parse(info->conn, info->rawData, info->dataLen);
        } else { hap_network_close(info->conn); }
        delete info;
        return;
    }

    auto pairInfo = info->session->pairInfo();

    //If isPairing or isVerifying, let PairingsManager handle this decryption
    if(pairInfo->isPairing){
        pairingsManager->onPairingDeviceDecryption(pairInfo, info->session);
    }

    if(pairInfo->isVerifying){
        pairingsManager->onVerifyingDeviceDecryption(pairInfo, info->session);
    }
}

void HAPServer::_onDataEncrypted(HAPEvent * event) {
    auto info = event->arg<hap_crypto_info>();

    if((info->flags) & CRYPTO_FLAG_NETWORK){ // NOLINT
        uint8_t frame[info->dataLen + 18];
        memcpy(frame, info->aad, 2);
        memcpy(frame + 2, info->encryptedData, info->dataLen + 16);
        delete info;
        hap_network_send(info->conn, frame, info->dataLen + 18);
        return;
    }

    auto pairInfo = info->session->pairInfo();

    if(pairInfo->isPairing){
        pairingsManager->onPairingDeviceEncryption(pairInfo, info->session);
    }

    if(pairInfo->isVerifying){
        pairingsManager->onVerifyingDeviceEncryption(pairInfo, info->session);
    }
}

void HAPServer::_onDevicePair(HAPEvent * event) {
    auto info = event->arg<hap_pair_info>();
    if(info->isPairing){
        pairingsManager->onDevicePaired(info, info->setupStore->session);
    }
}

void HAPServer::_onDeviceVerify(HAPEvent * event) {
    auto info = event->arg<hap_pair_info>();
    if(info->isVerifying){
        pairingsManager->onDeviceVerified(info, info->infoStore->session);
    }
}

void HAPServer::_onInitKeypairReq(HAPEvent *) {
    uint8_t pubKey[32], secKey[64];
    hap_crypto_longterm_keypair(pubKey, secKey);
    storage->setAccessoryLongTermKeys(pubKey, secKey);

    HAP_DEBUG("New keypair generated.");
    hexdump(pubKey, 32);
    hexdump(secKey, 64);
}

void HAPServer::onInboundData(hap_network_connection *client, uint8_t *body, uint8_t *tag, unsigned int bodyLen) {
    auto user = client->user;
    auto info = user->pair_info;
    auto aad = new uint8_t[2];
    aad[0] = static_cast<uint8_t>(bodyLen % 0xff);
    aad[1] = static_cast<uint8_t>(bodyLen / 0xff);

    //Prepare read context
    auto crypto = info->prepare(false, client);
    crypto->encryptedData = body;
    crypto->dataLen = bodyLen;
    crypto->authTag = tag;
    crypto->aadLen = 2;
    crypto->aad = aad;
    hap_crypto_data_decrypt(crypto);
}

void HAPServer::onOutboundData(hap_network_connection * client, uint8_t *body, unsigned int bodyLen) {
    auto user = client->user;
    auto info = user->pair_info;
    auto aad = new uint8_t[2];
    aad[0] = static_cast<uint8_t>(bodyLen % 0xff);
    aad[1] = static_cast<uint8_t>(bodyLen / 0xff);

    //Write context
    auto crypto = info->prepare(true, client);
    crypto->rawData = body;
    crypto->dataLen = bodyLen;
    crypto->aadLen = 2;
    crypto->aad = aad;
    hap_crypto_data_encrypt(crypto);
}

void HAPServer::_sendAttributionDatabase(HAPUserHelper * request) {
    SCONST uint8_t _begin[] = "{\"accessories\":[";
    SCONST uint8_t _end[] = "]}";
    SCONST unsigned int _beginLen = sizeof(_begin) - 1;
    SCONST unsigned int _endLen = sizeof(_end) - 1;
    SCONST unsigned int _wrpLen = sizeof(_begin) + sizeof(_end) - 2;

    HAPSerializeOptions options;
    options.withEv = false;
    options.withMeta = true;
    options.withPerms = true;
    options.withType = true;

    auto another = new HAPUserHelper(nullptr);
    auto v = request == another;

    unsigned int resLen = _wrpLen;
    auto current = accessories;

    while (current != nullptr){
//        resLen += current->serializeLength(&options);
        current = current->next;
        resLen += current == nullptr ? 0 : 1;
    }

    auto buf = new uint8_t[resLen];
    auto bufPtr = buf;

    memcpy(bufPtr, _begin, _beginLen);
    bufPtr += _beginLen;

    current = accessories;
    while (current != nullptr){
//        current->serialize(&bufPtr, &options);
        current = current->next;
        if(current != nullptr){
            *bufPtr = ',';
            ++bufPtr;
        }
    }

    memcpy(bufPtr, _end, _endLen);
    bufPtr += _endLen;

    request->setContentType(HAP_JSON);
    request->send(buf, resLen);
    delete[] buf;
}

void HAPServer::addAccessory(BaseAccessory * accessory) {
    auto current = &accessories;
    while ((*current) != nullptr){ current = &((*current)->next); }
    *current = accessory;
}

BaseAccessory * HAPServer::getAccessory(unsigned int aid) {
    auto current = accessories;
    while (current != nullptr && current->accessoryIdentifier != aid){
        current = current->next;
    }
    return current;
}
