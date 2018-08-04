//
// Created by Xule Zhou on 6/4/18.
//

#include "network.h"
#include "HomeKitAccessory.h"
#include "hap_crypto.h"

#include <cstring>
#include <cmath>
#include <cstdlib>

static const char _method_get[] PROGMEM = "GET";
static const char _method_put[] PROGMEM = "PUT";
static const char _method_post[] PROGMEM = "POST";
static const char _path_accessories[] PROGMEM = "/accessories";
static const char _path_characteristics[] PROGMEM = "/characteristics";
static const char _path_identify[] PROGMEM = "/identify";
static const char _path_pair_setup[] PROGMEM = "/pair-setup";
static const char _path_pair_verify[] PROGMEM = "/pair-verify";
static const char _path_pairings[] PROGMEM = "/pairings";
static const char _header_content_type[] PROGMEM = "Content-Type";
static const char _header_content_length[] PROGMEM = "Content-Length";
static const char _header_host[] PROGMEM = "Host";
static const char _ctype_tlv8[] PROGMEM = "application/pairing+tlv8";
static const char _ctype_json[] PROGMEM = "application/hap+json";
static const char _message_http11[] PROGMEM = "HTTP/1.1";
static const char _message_event10[] PROGMEM = "EVENT/1.0";

//static const char _status_100[] PROGMEM = "100 Continue";

//5.7.1.1 Successful HTTP Status Codes
static const char _status_200[] PROGMEM = "200 OK";
static const char _status_204[] PROGMEM = "204 No Content";
static const char _status_207[] PROGMEM = "207 Multi-Status";

//5.7.1.2 Client Error HTTP Status Codes
static const char _status_400[] PROGMEM = "400 Bad Request"; //HAP client error, e.g. a malformed request
static const char _status_404[] PROGMEM = "404 Not Found"; //An invalid URL
static const char _status_422[] PROGMEM = "422 Unprocessable Entity"; //For a well-formed request that contains invalid parameters

//A few 4xx additional from Table 4-2
static const char _status_429[] PROGMEM = "429 Too Many Requests";
static const char _status_470[] PROGMEM = "470 Connection Authorization Required";

//5.7.1.3 Server Error Status HTTP Codes
static const char _status_500[] PROGMEM = "500 Internal Server Error";
static const char _status_503[] PROGMEM = "503 Service Unavailable";

/**
 * Called when a new TCP connection is being accepted.
 */
void hap_event_network_accept(hap_network_connection *server, hap_network_connection *client) {
    auto user = new hap_user_connection();
    auto hap = server->server;
    user->request_buffer = nullptr;
    user->request_header = nullptr;
    user->response_header = nullptr;
    user->response_buffer = nullptr;
    user->frameBuf = nullptr;
    user->frameBufCurrLen = 0;
    user->frameExpLen = 0;
    client->user = user;
    client->server = hap;
    hap->emit(HAPEvent::HAP_NET_CONNECT, client);
}

void hap_http_parse(hap_network_connection *client, const uint8_t *originalData, unsigned int length){
    auto user = client->user;
    auto data = originalData;

    if(user->response_header && user->response_header->content_length == user->request_current_length){
        HAP_DEBUG("Not flushed when new data arrives. Cleaning right now.");
        //Clean buffer before creating new ones
        hap_network_flush(client);
    }

#define CMP_PROGSTR_PTR(prog) (strncasecmp_P(reinterpret_cast<const char *>(data), prog, strlen_P(prog)) == 0)

    // If request_header has been not received, it means its time to create a new request!
    if (user->request_header == nullptr) {
        //Fail if length is smaller than 125
        if (length < 60) {
            HAP_DEBUG("Header length (%u) too short. Ignoring current packet.", length);
            return;
        }

        user->request_header = new hap_http_description();
        user->request_header->content_length = 0;
        user->request_buffer = nullptr;

        //Initialize response buffers
        delete user->response_header;
        user->response_header = new hap_http_description();
        user->response_buffer = nullptr;

        //Parse request method
        // "GET /accessories HTTP/1.1\r\n"

        if (CMP_PROGSTR_PTR(_method_put)) {
            user->request_header->method = PUT;
            data += strlen_P(_method_put) + 1;
        } else if (CMP_PROGSTR_PTR(_method_get)) {
            user->request_header->method = GET;
            data += strlen_P(_method_get) + 1;
        } else if (CMP_PROGSTR_PTR(_method_post)) {
            user->request_header->method = POST;
            data += strlen_P(_method_post) + 1;
        } else {
            HAP_DEBUG("Unknown request method. Ignoring current packet.");
            delete user->request_header;
            user->request_header = nullptr;
            return;
        }

        //Parse request path

        if (CMP_PROGSTR_PTR(_path_accessories)) {
            user->request_header->path = ACCESSORIES;
            data += strlen_P(_path_accessories);
        } else if (CMP_PROGSTR_PTR(_path_characteristics)) {
            user->request_header->path = CHARACTERISTICS;
            data += strlen_P(_path_characteristics);
        } else if (CMP_PROGSTR_PTR(_path_identify)) {
            user->request_header->path = IDENTIFY;
            data += strlen_P(_path_identify);
        } else if (CMP_PROGSTR_PTR(_path_pair_setup)) {
            user->request_header->path = PAIR_SETUP;
            data += strlen_P(_path_pair_setup);
        } else if (CMP_PROGSTR_PTR(_path_pair_verify)) {
            user->request_header->path = PAIR_VERIFY;
            data += strlen_P(_path_pair_verify);
        } else if (CMP_PROGSTR_PTR(_path_pairings)) {
            user->request_header->path = PAIRINGS;
            data += strlen_P(_path_pairings);
        } else {
            //TODO: send 404
            HAP_DEBUG("Unacceptable request path. Ignoring current packet.");
            delete user->request_header;
            user->request_header = nullptr;
            return;
        }

        data += 11;//Skip the " HTTP/1.1\r\n"

        while ((data - originalData) < length) {
            //End of request_header section
            if ((data + 1 - originalData) < length && *data == '\r' && *(data + 1) == '\n') {
                data += 2;//Skip to body
                break;
            }

            auto key = new char[16]();
            auto value = new char[64]();

            auto keyPtr = key;
            while (*data != ':' && (data - originalData) < length) {
                *keyPtr = *data;
                ++keyPtr;
                ++data;
            }

            if (*data == ':' && (data - originalData) < length) ++data;
            if (*data == ' ' && (data - originalData) < length) ++data;

            auto valPtr = value;
            while ((data + 1 - originalData) < length && *data != '\r' && *(data + 1) != '\n') {
                *valPtr = *data;
                ++valPtr;
                ++data;
            }

            if (strncasecmp_P(key, _header_content_length, strlen_P(_header_content_length)) == 0) {
                user->request_header->content_length = static_cast<unsigned int>(atoi(value));
            } else if (strncasecmp_P(key, _header_content_type, strlen_P(_header_content_type)) == 0) {
                //parse content types
                if (strncasecmp_P(value, _ctype_tlv8, strlen_P(_ctype_tlv8)) == 0)
                    user->request_header->content_type = HAP_PAIRING_TLV8;
                else if (strncasecmp_P(value, _ctype_json, strlen_P(_ctype_json)) == 0)
                    user->request_header->content_type = HAP_JSON;
                else
                    HAP_DEBUG("unknown content type: %s", value);
            } else if (strncasecmp_P(key, _header_host, strlen_P(_header_host)) == 0) {
                auto bufLen = strlen(value) + 1;
                auto hostBuf = new char[bufLen]();
                memcpy(hostBuf, value, bufLen);
                if (user->request_header->host) {
                    auto previousBuf = user->request_header->host;
                    delete[] previousBuf;
                    user->request_header->host = nullptr;
                }
                user->request_header->host = hostBuf;
            }

            data += 2;//Next line
            delete[] key;
            delete[] value;
        }
    }

    //Creates content buffer
    if (user->request_buffer == nullptr) {
        user->request_buffer = new uint8_t[user->request_header->content_length]();
        user->request_current_length = 0;
    }

    //If we have more data, append to the buffer
    auto available = length - (data - originalData);
    auto needed = user->request_header->content_length - user->request_current_length;
    if (available > 0) {
        auto copyLength = available > needed ? needed : available;
        memcpy(user->request_buffer + user->request_current_length, data, copyLength);
        user->request_current_length += copyLength;
    }

    //If we have read all the data we need
    if (user->request_current_length == user->request_header->content_length) {
        client->server->emit(HAPEvent::HAP_NET_RECEIVE_REQUEST, client);
    }
}

/**
 * Called when data is received.
 *
 * @param data Pointer to the data
 * @param length Length of the data
 */
void hap_event_network_receive(hap_network_connection *client, const uint8_t *data, unsigned int left) {
    auto user = client->user;
    auto info = user->pair_info;

    if(info->paired()) {
        if(!user->frameBuf){ user->frameBuf = new uint8_t[1024]; }

        while (left > 0){
            //New frame starts here
            if(user->frameExpLen == 0){
                user->frameExpLen = data[0] + (static_cast<unsigned int>(data[1]) * 0xff);
                left -= 2;
                data += 2;
            }

            auto need = (user->frameExpLen + 16) - user->frameBufCurrLen;
            auto copy = need > left ? left : need;
            memcpy(user->frameBuf + user->frameBufCurrLen, data, copy);
            left -= copy;
            data += copy;
            user->frameBufCurrLen += copy;

            if(user->frameBufCurrLen == (user->frameExpLen + 16)){
                //hap crypto will free this buffer when decryption succeed
                auto buf = new uint8_t[user->frameExpLen + 16];
                memcpy(buf, user->frameBuf, user->frameExpLen + 16);
                client->server->onEncryptedData(client, buf, buf + user->frameExpLen, user->frameExpLen);
                user->frameExpLen = 0;
                user->frameBufCurrLen = 0;
            }
        }
    } else { hap_http_parse(client, data, left); }
}

static void hap_user_flush(hap_user_connection * user){
    auto header = user->request_header;
    if (header) {
        if (header->host) {
            delete[] header->host;
            header->host = nullptr;
        }
        user->request_header = nullptr;
        delete header;
    }
    auto buf = user->request_buffer;
    if (buf) {
        user->request_buffer = nullptr;
        user->request_current_length = 0;
        delete[] buf;
    }
}

/**
 * Called when connection is closed.
 */
void hap_event_network_close(hap_network_connection *client) {
    auto user = client->user;
    client->server->emit(HAPEvent::HAP_NET_DISCONNECT, user, [](HAPEvent * e){
        auto u = e->arg<hap_user_connection>();
        hap_user_flush(u);
    });
}

/**
 * Flush request
 */
void hap_network_flush(hap_network_connection *client) {
    auto user = client->user;
    if (user) { hap_user_flush(user); }
}

void hap_network_response(hap_network_connection *client) {
    auto user = client->user;
    auto header = user->response_header;
    const char *status_ptr, *msg_type_ptr, *ctype_ptr;

#define _CASE_STATUS(stat) \
    case stat: \
        status_ptr = _status_ ## stat; \
        break;

    //status code
    switch (header->status) {
//        _CASE_STATUS(100)
        _CASE_STATUS(200)
        _CASE_STATUS(204)
        _CASE_STATUS(207)
        _CASE_STATUS(400)
        _CASE_STATUS(404)
        _CASE_STATUS(422)
        _CASE_STATUS(429)
        _CASE_STATUS(470)
        _CASE_STATUS(500)
        _CASE_STATUS(503)
        default:
            status_ptr = _status_200;
            HAP_DEBUG("Unknown status in response: %u", header->status);
    }

    //message type
    switch (header->message_type){
        case EVENT_1_0:
            msg_type_ptr = _message_event10;
            break;
        case HTTP_1_1:
        default:
            msg_type_ptr = _message_http11;
    }

    //other headers

    switch (header->content_type){
        case HAP_PAIRING_TLV8:
            ctype_ptr = _ctype_tlv8;
            break;
        case HAP_JSON:
            ctype_ptr = _ctype_json;
            break;
        default:
            HAP_DEBUG("Unknown content type: %d. Not sending this response.", header->content_type);
            return;
    }

    auto frame_buf = new char[1024]();
    auto frame_ptr = frame_buf;

#define _NEWCPY(name, ptr) \
    auto (name) = new char[strlen_P(ptr)](); \
    strcpy_P(name, ptr);

    _NEWCPY(msg_type, msg_type_ptr);
    _NEWCPY(status, status_ptr);
    _NEWCPY(ctype_hdr, _header_content_type);
    _NEWCPY(ctype_val, ctype_ptr);
    _NEWCPY(clen_hdr, _header_content_length);

    frame_ptr += snprintf_P(frame_ptr, 1024,
               "%s %s\r\n%s: %s\r\n%s: %u\r\n\r\n",
               msg_type, status,
               ctype_hdr, ctype_val,
               clen_hdr, header->content_length
    );

    delete[] msg_type;
    delete[] status;
    delete[] ctype_hdr;
    delete[] ctype_val;
    delete[] clen_hdr;

    auto bodyPtr = user->response_buffer;
    while ((bodyPtr - user->response_buffer) < header->content_length){
        auto spaceLen = 1024 - (frame_ptr - frame_buf);
        auto leftLen = header->content_length - (bodyPtr - user->response_buffer);
        auto copyLen = spaceLen > leftLen ? leftLen : spaceLen;
        memcpy(frame_ptr, bodyPtr, copyLen);
        bodyPtr += copyLen;
        frame_ptr += copyLen;
        hap_network_send(client, reinterpret_cast<const uint8_t *>(frame_buf),
                         static_cast<unsigned int>(frame_ptr - frame_buf));
        memset(frame_buf, 0, 1024);
        frame_ptr = frame_buf;
    }

    if(frame_ptr != frame_buf){
        hap_network_send(client, reinterpret_cast<const uint8_t *>(frame_buf),
                         static_cast<unsigned int>(frame_ptr - frame_buf));
    }

    delete[] frame_buf;
}
