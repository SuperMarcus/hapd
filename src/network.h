//
// Created by Xule Zhou on 6/4/18.
//

#ifndef ARDUINOHOMEKIT_NETWORK_H
#define ARDUINOHOMEKIT_NETWORK_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hap_network_connection {
    void * raw;
};

struct hap_user_connection {
    hap_network_connection connection;

};

/**
 * The following functions are designed to be implemented by platforms.
 *
 * The network functions of ArduinoHomeKit is driven by events sent by implementations. This
 * means that the implementations are suppose to call the handlers above.
 */

/**
 * Tells the network implementation to create a socket
 *
 * @return If the socket is successfully created, return true
 */
extern bool hap_network_init(hap_network_connection *);

/**
 * Tells the network implementation to bind to a specific port
 *
 * @param port The port to bind to
 * @return If success, return true
 */
extern bool hap_network_bind(hap_network_connection *, uint16_t port);

/**
 * The following functions are designed to be called by network implementations when an
 * event is triggered.
 */

/**
 * Called when a new TCP connection is being accepted.
 */
void hap_event_network_accept(hap_network_connection *);

/**
 * Called when data is received.
 *
 * @param data Pointer to the data
 * @param length Length of the data
 */
void hap_event_network_receive(hap_network_connection *, char * data, unsigned int length);

/**
 * Called when connection is closed.
 */
void hap_event_network_close(hap_network_connection *);

#ifdef __cplusplus
}
#endif

#endif //ARDUINOHOMEKIT_NETWORK_H
