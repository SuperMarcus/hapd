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

#ifndef HAPD_NETWORK_H
#define HAPD_NETWORK_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 * Init mDNS service discover
 *
 * The implementation should register the hap service and return the service ref
 *
 * @param name of the accessory
 * @return null if an error occurs; otherwise a pointer to the data structure that the implementation use
 */
extern void * hap_service_discovery_init(const char * name, uint16_t port);

/**
 * Update mDNS service
 *
 * @param Point to the handle returned by hap_service_discovery_init()
 */
extern bool hap_service_discovery_update(void *, hap_sd_txt_item *);

/**
 *
 */
extern void hap_service_discovery_loop(void *);

/**
 * Terminate mDNS broadcast
 *
 * Called when HAPServer instance is destroyed
 */
extern void hap_service_discovery_deinit(void *);

/**
 * The following functions are designed to be called by network
 * implementations when an event is triggered, or used by HAPServer for
 * request handling.
 */

/**
 * Called when a new TCP connection is being accepted.
 */
void hap_event_network_accept(hap_network_connection * server, hap_network_connection * client);

/**
 * Called when data is received.
 *
 * @param data Pointer to the data
 * @param length Length of the data
 */
void hap_event_network_receive(hap_network_connection * client, const uint8_t * data, unsigned int length);

/**
 * Called when connection is closed.
 *
 * This handler should be called both when the client and the server
 * initiate the shutdown.
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

/**
 * Parse http data
 *
 * @param client
 * @param data
 * @param length
 */
void hap_http_parse(hap_network_connection *client, const uint8_t *data, unsigned int length);

#ifdef __cplusplus
}
#endif

#endif //HAPD_NETWORK_H
