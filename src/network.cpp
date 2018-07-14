//
// Created by Xule Zhou on 6/4/18.
//

#include "network.h"
#include "HomeKitAccessory.h"

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
static const char _header_content_type[] PROGMEM = "content-type";
static const char _header_content_length[] PROGMEM = "content-length";
static const char _header_host[] PROGMEM = "host";
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

//5.7.1.3 Server Error Status HTTP Codes
static const char _status_500[] PROGMEM = "500 Internal Server Error";
static const char _status_503[] PROGMEM = "503 Service Unavailable";

/**
 * Called when a new TCP connection is being accepted.
 */
void hap_event_network_accept(hap_network_connection *server, hap_network_connection *client) {
    auto user = new hap_user_connection();
    user->request_buffer = nullptr;
    user->request_header = nullptr;
    client->user = user;
    client->request_cb = server->request_cb;
    client->request_cb_arg = server->request_cb_arg;
}

/**
 * Called when data is received.
 *
 * @param data Pointer to the data
 * @param length Length of the data
 */
void hap_event_network_receive(hap_network_connection *client, const uint8_t *originalData, unsigned int length) {
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
        delete user->response_buffer;
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

    //TODO: see if there is a need for 100
//    if (user->request_header->content_length > 0) {
//        user->response_header->status = 100;
//        user->response_header->message_type = HTTP_1_1;
//        hap_network_response(client);
//    }

    //Creates content buffer
    if (user->request_buffer == nullptr) {
        user->request_buffer = new uint8_t[user->request_header->content_length]();
        user->request_current_length = 0;
    }

    //If we have more data, append to the buffer
    auto available = data - originalData;
    auto needed = user->request_header->content_length - user->request_current_length;
    if (available > 0) {
        auto copyLength = available > needed ? needed : available;
        memcpy(user->request_buffer + user->request_current_length, data, copyLength);
        user->request_current_length += copyLength;

        //If we have read all the data we need
        if (user->request_current_length == user->request_header->content_length) {
            client->request_cb(client, client->request_cb_arg);
        }
    }
}

/**
 * Called when connection is closed.
 */
void hap_event_network_close(hap_network_connection *client) {
    hap_network_flush(client);
    delete client->user;
    client->user = nullptr;
}

/**
 * Flush request
 */
void hap_network_flush(hap_network_connection *client) {
    auto user = client->user;
    if (user) {
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
}

void hap_network_response(hap_network_connection *client) {
    auto user = client->user;
    auto header = user->response_header;
    const char *status_ptr, *msg_type_ptr, *ctype_ptr;
    size_t header_length = 0;

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
        _CASE_STATUS(500)
        _CASE_STATUS(503)
        default:
            status_ptr = _status_200;
            HAP_DEBUG("Unknown status in response: %u", header->status);
    }
    header_length += strlen_P(status_ptr) + 1;

    //message type
    switch (header->message_type){
        case EVENT_1_0:
            msg_type_ptr = _message_event10;
            break;
        case HTTP_1_1:
        default:
            msg_type_ptr = _message_http11;
    }
    header_length += strlen_P(msg_type_ptr) + 2;//with \r\n

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
    header_length += strlen_P(_header_content_type) + strlen_P(ctype_ptr) + 4;//4 bytes = ": " and "\r\n"
    header_length += strlen_P(_header_content_length) + floor(log10(header->content_length)) + 5;//5 bytes = same above + 1 for number
    header_length += 2;//for "\r\n" at the end

    auto header_buf = new char[header_length + 1]();
    snprintf_P(header_buf, header_length + 1,
#ifdef NATIVE_STRINGS
               "%s %s\r\n%s: %s\r\n%s: %u\r\n\r\n",
#else
               PSTR("%S %S\r\n%S: %S\r\n%S: %u\r\n\r\n"),
#endif
               msg_type_ptr, status_ptr,
               _header_content_type, ctype_ptr,
               _header_content_length, header->content_length
    );

    //send header
    hap_network_send(client, reinterpret_cast<const uint8_t *>(header_buf), static_cast<unsigned int>(header_length));
    delete[] header_buf;

    //send user data if there is one
    if(header->content_length > 0 && user->response_buffer != nullptr)
    { hap_network_send(client, user->response_buffer, header->content_length); }
}
