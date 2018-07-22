//
// Created by Xule Zhou on 6/4/18.
//

#ifndef ARDUINOHOMEKIT_COMMON_H
#define ARDUINOHOMEKIT_COMMON_H

#include "platform.h"

#include <cstdbool>
#include <cstdint>
#include <cstdlib>

#ifndef HAP_DEBUG

#if defined(USE_HARDWARE_SERIAL)
#include <HardwareSerial.h>
#define HAP_DEBUG(message, ...) Serial.printf_P(PSTR( __FILE__ ":%d [%s] " message "\n", __LINE__ ), __FUNCTION__, ##__VA_ARGS__)
#elif defined(USE_PRINTF)
#include <cstdio>
#define HAP_DEBUG(message, ...) fprintf(stdout, __FILE__ ":%d [%s] " message "\n", __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define HAP_DEBUG(message, ...)
#endif

#endif

#ifndef HAP_SOCK_TCP_NODELAY
//Set to 0 to disable tcp nodelay
#define HAP_SOCK_TCP_NODELAY 1
#endif

#ifndef HAP_NOTHING
#define HAP_NOTHING
#endif

#define HTTP_200_OK                     200
#define HTTP_204_NO_CONTENT             204
#define HTTP_207_MULTI_STATUS           207
#define HTTP_400_BAD_REQUEST            400
#define HTTP_404_NOT_FOUND              404
#define HTTP_422_UNPROCESSABLE_ENTITY   422
#define HTTP_429_TOO_MANY_REQUESTS      429
#define HTTP_470_CONN_AUTH_REQUIRED     470
#define HTTP_500_INTERNAL_ERROR         500
#define HTTP_503_SERVICE_UNAVAILABLE    503

class HAPServer;
struct hap_pair_info;
struct hap_crypto_setup;

enum hap_msg_type {
    MESSAGE_TYPE_UNKNOWN, HTTP_1_1, EVENT_1_0
};

enum hap_http_method {
    METHOD_UNKNOWN, GET, PUT, POST
};

enum hap_http_path {
    PATH_UNKNOWN,
    ACCESSORIES,
    CHARACTERISTICS,
    IDENTIFY,
    PAIR_SETUP,
    PAIR_VERIFY,
    PAIRINGS
};

enum hap_http_content_type {
    CONTENT_TYPE_UNKNOWN,
    HAP_PAIRING_TLV8, // application/pairing+tlv8
    HAP_JSON // application/hap+json
};

//Contains all the headers needed
struct hap_http_description {
    hap_msg_type message_type = HTTP_1_1;
    hap_http_method method = METHOD_UNKNOWN;
    hap_http_path path = PATH_UNKNOWN;
    const char * host = nullptr;
    uint16_t status = 204;//Default to 204 no content
    unsigned int content_length = 0;
    hap_http_content_type content_type = CONTENT_TYPE_UNKNOWN;
};

//Contains information for requests & responses
struct hap_user_connection {
    hap_http_description * request_header;
    uint8_t * request_buffer;
    unsigned int request_current_length;
    hap_pair_info * pair_info;

    hap_http_description * response_header;
    uint8_t * response_buffer;
};

struct hap_network_connection {
    void * raw;
    HAPServer * server;
    hap_user_connection * user;
};

struct hap_sd_txt_item {
    const char * key;
    const char * value;
};

#endif //ARDUINOHOMEKIT_COMMON_H
