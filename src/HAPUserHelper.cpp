/**
 * hapd
 *
 * Copyright 2018 Xule Zhou
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "common.h"
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
    send();
}

void HAPUserHelper::send(int status, hap_msg_type type) {
    setResponseStatus(status);
    if(type != MESSAGE_TYPE_UNKNOWN) setResponseType(type);
    send();
}

void HAPUserHelper::send(const char *body) {
    auto contentLength = static_cast<unsigned int>(strlen(body));
    send(reinterpret_cast<const void*>(body), contentLength);
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
    if(refCount == 0){
        delete this;
    }
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

void HAPUserHelper::sendError(uint8_t error, int status) {
    auto state = pairInfo()->currentStep;
    auto res = tlv8_insert(nullptr, kTLVType_Error, 1, &error);
    tlv8_insert(res, kTLVType_State, 1, &state);
    setResponseStatus(status);
    send(res);
}

void HAPUserHelper::send() {
    hap_network_response(conn);
}

const hap_http_request_parameters *HAPUserHelper::params() {
    return &conn->user->request_header->parameters;
}
