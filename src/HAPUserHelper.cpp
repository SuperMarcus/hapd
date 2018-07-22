#include "HomeKitAccessory.h"
#include "network.h"
#include "tlv.h"
#include <cstring>

HAPUserHelper::HAPUserHelper(hap_network_connection *conn):
        conn(conn), refCount(0){ }

void HAPUserHelper::setResponseStatus(int status) {
    conn->user->response_header->status = static_cast<uint16_t>(status);
}

void HAPUserHelper::setBody(const void *body, unsigned int contentLength) {
    conn->user->response_buffer = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(body));
    conn->user->response_header->content_length = contentLength;
    if(conn->user->response_header->status == 204) setResponseStatus(200);
}

void HAPUserHelper::setResponseType(hap_msg_type type) {
    conn->user->response_header->message_type = type;
}

void HAPUserHelper::send(const void *body, unsigned int contentLength) {
    //TODO: remove
    extern void hexdump(const void *ptr, int buflen);
    if(body != nullptr && contentLength > 0) setBody(body, contentLength);
    //TODO: remove
    HAP_DEBUG("HTTP Message sending...status %d, len %d", conn->user->response_header->status, conn->user->response_header->content_length);
    hexdump(body, contentLength);
    hap_network_response(conn);
}

void HAPUserHelper::send(int status, hap_msg_type type) {
    setResponseStatus(status);
    if(type != MESSAGE_TYPE_UNKNOWN) setResponseType(type);
    send();
}

void HAPUserHelper::send(const char *body, int contentLength) {
    if(contentLength < 0){ contentLength = static_cast<int>(strlen(body)); }
    send(reinterpret_cast<const void*>(body), static_cast<unsigned int>(contentLength));
}

void HAPUserHelper::send(tlv8_item * chain) {
    unsigned int len;
    auto buf = tlv8_export_free(chain, &len);
    if(conn->user->response_header->content_type == CONTENT_TYPE_UNKNOWN) setContentType(HAP_PAIRING_TLV8);
    if(conn->user->response_header->status == 204) setResponseStatus(200);
    send(buf, len);
    delete[] buf;
}

void HAPUserHelper::setContentType(hap_http_content_type ctype) {
    conn->user->response_header->content_type = ctype;
}

HAPUserHelper::~HAPUserHelper() {
    //Clean buffer after helper is destroyed
    hap_network_flush(conn);
    conn->user->response_buffer = nullptr;
    conn = nullptr;
    HAP_DEBUG("Session finished.");
}

void HAPUserHelper::retain() {
    ++refCount;
}

void HAPUserHelper::release() {
    --refCount;
    if(refCount == 0){ delete this; }
}

unsigned int HAPUserHelper::dataLength() {
    return conn->user->request_current_length;
}

hap_http_path HAPUserHelper::path() {
    return conn->user->request_header->path;
}

hap_http_content_type HAPUserHelper::requestContentType() {
    return conn->user->request_header->content_type;
}

hap_http_method HAPUserHelper::method() {
    return conn->user->request_header->method;
}

void HAPUserHelper::close() {
    hap_network_close(conn);
}

hap_pair_info * HAPUserHelper::pairInfo() {
    return conn->user->pair_info;
}
