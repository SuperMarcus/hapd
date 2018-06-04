//
// Created by Xule Zhou on 6/4/18.
//

#include "network.h"

/**
 * Called when a new TCP connection is being accepted.
 */
void hap_event_network_accept(hap_network_connection * client){

}

/**
 * Called when data is received.
 *
 * @param data Pointer to the data
 * @param length Length of the data
 */
void hap_event_network_receive(hap_network_connection * client, const uint8_t * data, unsigned int length){

}

/**
 * Called when connection is closed.
 */
void hap_event_network_close(hap_network_connection * client){

}
