//
// Created by Xule Zhou on 6/4/18.
//

#ifndef ARDUINOHOMEKIT_NETWORK_H
#define ARDUINOHOMEKIT_NETWORK_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    TYPE_UNKNOWN,
    HAP_PAIRING_TLV8, // application/pairing+tlv8
    HAP_JSON // application/hap+json
};

struct hap_http_description {
    hap_http_method method = METHOD_UNKNOWN;
    hap_http_path path = PATH_UNKNOWN;
    const char * host = nullptr;
    uint16_t status = 0;
    unsigned int content_length = 0;
    hap_http_content_type content_type = TYPE_UNKNOWN;
};

struct hap_user_connection {
    hap_http_description * header;
    uint8_t * request_buffer;
    unsigned int request_current_length;
};

struct hap_network_connection {
    void * raw;
    void (*request_cb)(hap_network_connection *, void *);
    void * request_cb_arg;
    hap_user_connection * user;
};

/**
 * For future ports to other platforms, the following functions are designed
 * to be implemented by platforms.
 *
 * The network functions of ArduinoHomeKit is driven by events sent by
 * implementations. This means that the implementations are suppose to
 * call the handlers above.
 */

/**
 * Tells the network implementation to create a socket and bind to a specific port
 *
 * @param port The port to bind to
 * @return If the socket is successfully created, return true
 */
extern bool hap_network_init_bind(hap_network_connection *, uint16_t port);

/**
 * Send data to a network connection
 *
 * @param client The connection to send data on
 * @param data Pointer to data
 * @param length Length of data
 * @return If success, return true
 */
extern bool hap_network_send(hap_network_connection * client, const uint8_t * data, unsigned int length);

/**
 * Close the target connection
 *
 * This asks the network implementation to close the tcp connection and
 * free up any memories used for this connection.
 *
 * @param client The connection to be closed
 * @return
 */
extern void hap_network_close(hap_network_connection * client);

/**
 * Polling off events from the network
 */
extern void hap_network_loop();

/**
 * The following functions are designed to be called by network
 * implementations when an event is triggered.
 */

/**
 * Called when a new TCP connection is being accepted.
 */
void hap_event_network_accept(hap_network_connection * client);

/**
 * Called when data is received.
 *
 * @param data Pointer to the data
 * @param length Length of the data
 */
void hap_event_network_receive(hap_network_connection * client, const uint8_t * data, unsigned int length);

/**
 * Called when connection is closed.
 */
void hap_event_network_close(hap_network_connection * client);

/**
 * Delete the current request and response buffers so we can receive more requests
 *
 * @param client
 */
void hap_network_flush(hap_network_connection * client);

/**
 * Send http response to the client
 *
 * @param client
 */
void hap_network_response(hap_network_connection * client);

#ifdef __cplusplus
}
#endif

#endif //ARDUINOHOMEKIT_NETWORK_H
