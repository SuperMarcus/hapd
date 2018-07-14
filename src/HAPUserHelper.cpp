#include "HomeKitAccessory.h"
#include <cstring>

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
