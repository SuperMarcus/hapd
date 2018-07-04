//
// Created by Xule Zhou on 6/4/18.
//

#include "network.h"
#include "HAPServer.h"

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

/**
 * Called when a new TCP connection is being accepted.
 */
void hap_event_network_accept(hap_network_connection * client){
    auto user = new hap_user_connection;
    user->request_buffer = nullptr;
    user->header = nullptr;
    client->user = user;
}

/**
 * Called when data is received.
 *
 * @param data Pointer to the data
 * @param length Length of the data
 */
void hap_event_network_receive(hap_network_connection * client, const uint8_t * originalData, unsigned int length){
    auto user = client->user;
    auto data = reinterpret_cast<const char *>(originalData);

#define CMP_PROGSTR_PTR(prog) (strncasecmp_P(data, prog, strlen_P(prog)) == 0)
#define _UI(v) reinterpret_cast<unsigned int>(v)

    // If header has been not received, it means its time to create a new request!
    if(user->header == nullptr){
        //Fail if length is smaller than 125
        if(length < 125){
            HAP_DEBUG("[net] Header length too short. Ignoring current packet.\n");
            return;
        }

        user->header = new hap_http_description();
        user->header->content_length = 0;
        user->request_buffer = nullptr;

        //Parse request method
        // "GET /accessories HTTP/1.1\r\n"

        if(CMP_PROGSTR_PTR(_method_put)){
            user->header->method = PUT;
            data += strlen_P(_method_put) + 1;
        } else if(CMP_PROGSTR_PTR(_method_get)) {
            user->header->method = GET;
            data += strlen_P(_method_get) + 1;
        } else if(CMP_PROGSTR_PTR(_method_post)) {
            user->header->method = POST;
            data += strlen_P(_method_post) + 1;
        } else {
            HAP_DEBUG("[net] Unknown request method. Ignoring current packet.\n");
            delete user->header;
            user->header = nullptr;
            return;
        }

        //Parse request path

        if(CMP_PROGSTR_PTR(_path_accessories)){
            user->header->path = ACCESSORIES;
            data += strlen_P(_path_accessories);
        } else if(CMP_PROGSTR_PTR(_path_characteristics)){
            user->header->path = CHARACTERISTICS;
            data += strlen_P(_path_characteristics);
        } else if(CMP_PROGSTR_PTR(_path_identify)){
            user->header->path = IDENTIFY;
            data += strlen_P(_path_identify);
        } else if(CMP_PROGSTR_PTR(_path_pair_setup)){
            user->header->path = PAIR_SETUP;
            data += strlen_P(_path_pair_setup);
        } else if(CMP_PROGSTR_PTR(_path_pair_verify)){
            user->header->path = PAIR_VERIFY;
            data += strlen_P(_path_pair_verify);
        } else if(CMP_PROGSTR_PTR(_path_pairings)){
            user->header->path = PAIRINGS;
            data += strlen_P(_path_pairings);
        } else {
            //TODO: send 404
            HAP_DEBUG("[net] Unacceptable request path. Ignoring current packet.\n");
            delete user->header;
            user->header = nullptr;
            return;
        }

        data += 11;//Skip the " HTTP/1.1\r\n"

        while((_UI(data) - _UI(originalData)) < length){
            //End of header section
            if((_UI(data) + 1 - _UI(originalData)) < length && *data == '\r' && *(data + 1) == '\n'){
                break;
            }

            auto key = new char[16]();
            auto value = new char[64]();

            auto keyPtr = key;
            while(*data != ':' && (_UI(data) - _UI(originalData)) < length){
                *keyPtr = *data;
                ++keyPtr;
                ++data;
            }

            auto valPtr = value;
            while((_UI(data) + 1 - _UI(originalData)) < length && *data != '\r' && *(data + 1) != '\n'){
                *valPtr = *data;
                ++valPtr;
                ++data;
            }

            if(strncasecmp_P(key, _header_content_length, strlen_P(_header_content_length)) == 0){
                user->header->content_length = static_cast<unsigned int>(atoi(value));
            }else if(strncasecmp_P(key, _header_content_type, strlen_P(_header_content_type)) == 0){
                //parse content types
                if(strncasecmp_P(value, _ctype_tlv8, strlen_P(_ctype_tlv8)) == 0)
                    user->header->content_type = HAP_PAIRING_TLV8;
                else if(strncasecmp_P(value, _ctype_json, strlen_P(_ctype_json)) == 0)
                    user->header->content_type = HAP_JSON;
                else HAP_DEBUG("[net] unknown content type: %s\n", value);
            }else if(strncasecmp_P(key, _header_host, strlen_P(_header_host)) == 0){
                auto bufLen = strlen(value) + 1;
                auto hostBuf = new char[bufLen]();
                memcpy(hostBuf, value, bufLen);
                if(user->header->host){
                    auto previousBuf = user->header->host;
                    delete[] previousBuf;
                    user->header->host = nullptr;
                }
                user->header->host = hostBuf;
            }

            data += 2;//Next line
            delete[] key;
            delete[] value;
        }
    }

    //Creates content buffer
    if(user->request_buffer == nullptr){
        user->request_buffer = new uint8_t[user->header->content_length]();
        user->request_current_length = 0;
    }

    //If we have more data, append to the buffer
    auto available = _UI(data) - _UI(originalData);
    auto needed = user->header->content_length - user->request_current_length;
    if(available > 0){
        auto copyLength = available > needed ? needed : available;
        memcpy(user->request_buffer + user->request_current_length, data, copyLength);
        user->request_current_length += copyLength;

        //If we have read all the data we need
        if(user->request_current_length == user->header->content_length){
            client->request_cb(client, client->request_cb_arg);
        }
    }
}

/**
 * Called when connection is closed.
 */
void hap_event_network_close(hap_network_connection * client){
    hap_network_flush(client);
    delete client->user;
}

/**
 * Flush request
 */
void hap_network_flush(hap_network_connection *client) {
    auto user = client->user;
    if(user){
        auto header = user->header;
        if(header){
            if(header->host){
                delete[] header->host;
                header->host = nullptr;
            }
            user->header = nullptr;
            delete header;
        }
        auto buf = user->request_buffer;
        if(buf){
            user->request_buffer = nullptr;
            user->request_current_length = 0;
            delete[] buf;
        }
    }
}
