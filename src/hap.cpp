#include <cstring>
#include "HAPServer.h"
#include "tlv.h"

HAPServer HomeKitAccessory;

void HAPServer::_onRequestReceived(hap_network_connection * conn) {
    auto request = new HAPUserHelper(conn);
    HAP_DEBUG("new message received: %s, length: %u", conn->user->request_buffer, conn->user->request_current_length);

    auto data = "This is the data that is going to be sent to the clients";

    request->setContentType(HAP_JSON);
    request->send(data);

    this->emit(HAPEvent::DUMMY, request, [](HAPEvent * event){
        auto req = static_cast<HAPUserHelper *>(event->argument);
        delete req;
    });
}

void HAPServer::_s_onRequestReceived(hap_network_connection *conn, void *arg) {
    static_cast<HAPServer *>(arg)->_onRequestReceived(conn);
}

void HAPServer::begin(uint16_t port) {
    server_conn = new hap_network_connection;
    server_conn->raw = nullptr;
    server_conn->user = nullptr;
    server_conn->request_cb_arg = this;
    server_conn->request_cb = &HAPServer::_s_onRequestReceived;

    hap_network_init_bind(server_conn, port);
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
                (*currentListener)(currentEvent);
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

void HAPServer::on(HAPEvent::EventID id, HAPEventListener::Callback cb) {
    auto listener = new HAPEventListener();
    listener->onEvent = cb;
    listener->listening = id;
    listener->next = nullptr;

    HAPEventListener ** appendListener = &eventListeners;
    while ((*appendListener) != nullptr) appendListener = &((*appendListener)->next);
    *appendListener = listener;
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

HAPUserHelper::HAPUserHelper(hap_network_connection *conn):
    conn(conn){ }

void HAPUserHelper::setResponseStatus(int status) {
    conn->user->response_header->status = static_cast<uint16_t>(status);
}

void HAPUserHelper::send(const void *body, unsigned int contentLength) {
    if(body != nullptr && contentLength > 0) setBody(body, contentLength);
    hap_network_response(conn);
}

void HAPUserHelper::setBody(const void *body, unsigned int contentLength) {
    conn->user->response_buffer = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(body));
    conn->user->response_header->content_length = contentLength;
    if(conn->user->response_header->status == 204) setResponseStatus(200);
}

void HAPUserHelper::send(int status, hap_msg_type type) {
    setResponseStatus(status);
    setResponseType(type);
    send();
}

void HAPUserHelper::setResponseType(hap_msg_type type) {
    conn->user->response_header->message_type = type;
}

void HAPUserHelper::send(const char *body, int contentLength) {
    if(contentLength < 0){ contentLength = static_cast<int>(strlen(body)); }
    send(reinterpret_cast<const void*>(body), static_cast<unsigned int>(contentLength));
}

void HAPUserHelper::setContentType(hap_http_content_type ctype) {
    conn->user->response_header->content_type = ctype;
}

HAPUserHelper::~HAPUserHelper() {
    //Clean buffer after helper is destroyed
    hap_network_flush(conn);
    conn = nullptr;
    HAP_DEBUG("Session finished.");
}

void HAPEventListener::operator()(HAPEvent * e) {
    if(onEvent) onEvent(e);
}
