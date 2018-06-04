/**
 * Network implementation for lwip
 */

#include <Arduino.h>

//We know esp8266 has lwip, so enable this implementation by default for esp8266

#if !defined(NO_BUILTIN_NETWORK_IMPLEMENTATION) && \
    defined(CORE_ESP8266_FEATURES_H) || \
    defined(USE_HAP_LWIP)

#include "network.h"
#include "common.h"

extern "C" {
#include <lwip/tcp.h>
};

#ifndef HAPLWIP_BIND_ADDR
#define HAPLWIP_BIND_ADDR IP_ADDR_ANY
#endif

#define HAPCONN_PCB(conn) (reinterpret_cast<tcp_pcb *>((conn)->raw))

/**
 * Events handlers for lwip
 */

/**
 * Sends RESET to the client and returns ERR_ABRT
 *
 * @return ERR_ABRT
 */
err_t _hap_lwip_abort(struct tcp_pcb * pcb){
    tcp_abort(pcb);
    return ERR_ABRT;
}

/**
 * Close the socket connection
 *
 * @return err_t values
 */
err_t _hap_lwip_close(struct tcp_pcb * pcb){
    if(pcb){
        tcp_arg(pcb, nullptr);
        tcp_sent(pcb, nullptr);
        tcp_recv(pcb, nullptr);
        tcp_err(pcb, nullptr);
        tcp_poll(pcb, nullptr, 0);

        auto result = tcp_close(pcb);
        if(result != ERR_OK){ return _hap_lwip_abort(pcb); }
    }
    return ERR_OK;
}

/**
 * Called when lwip receives data from clients
 *
 * @param client Client socket pcb wrapped in hap_network_connection*
 * @param tpcb Client pcb
 * @param p
 * @param err
 * @return OK normally
 */
err_t hap_lwip_receive(void * client, struct tcp_pcb * tpcb, struct pbuf * buffer, err_t err){
    auto hapconn = reinterpret_cast<hap_network_connection *>(client);
    if(buffer){
        struct pbuf * current;
        while (buffer != nullptr){
            current = buffer;

            HAP_DEBUG("[lwip] Received %u\n", current->len);
            hap_event_network_receive(hapconn, static_cast<const uint8_t *>(current->payload), current->len);
            tcp_recved(HAPCONN_PCB(hapconn), buffer->len);

            buffer = current->next;
            current->next = nullptr;
            pbuf_free(current);
        }
    } else { //Connection closed
        hap_event_network_close(hapconn);
        delete hapconn;
        return _hap_lwip_close(tpcb);
    }
    return ERR_OK;
}

/**
 * Called by lwip when application polling
 *
 * @param client Client socket pcb wrapped in hap_network_connection*
 * @param tpcb Client pcb
 * @return OK normally
 */
err_t hap_lwip_poll(void * client, struct tcp_pcb *tpcb){
    //TODO
    return ERR_OK;
}

/**
 * Called when an error occured
 *
 * @param client Client socket pcb wrapped in hap_network_connection*
 * @param err Error code
 */
void hap_lwip_error(void * client, err_t err){
    HAP_DEBUG("[lwip] lwip error: %ld\n", err);
}

/**
 * Called when lwip accepts a connection
 *
 * @param conn Listening socket pcb wrapped in hap_network_connection*
 * @param pcb New connection pcb
 * @param err Error
 * @return OK normally
 */
err_t hap_lwip_accept(void * conn, tcp_pcb * pcb, err_t err){
    auto * client_conn = new hap_network_connection;
    client_conn->raw = pcb;

    tcp_accepted(HAPCONN_PCB(client_conn));
#if HAP_LWIP_TCP_NODELAY
    tcp_nagle_disable(HAPCONN_PCB(client_conn));
#else
    tcp_nagle_enable(HAPCONN_PCB(client_conn));
#endif
    tcp_arg(HAPCONN_PCB(client_conn), client_conn);
    tcp_recv(HAPCONN_PCB(client_conn), &hap_lwip_receive);
    tcp_err(HAPCONN_PCB(client_conn), &hap_lwip_error);
    tcp_poll(HAPCONN_PCB(client_conn), &hap_lwip_poll, 1);

    hap_event_network_accept(client_conn);

    return ERR_OK;
}

/**
 * Implementations for hap_network
 */

/**
 * Bind to a specific port
 *
 * @param conn
 * @param port
 * @return
 */
bool hap_network_init_bind(hap_network_connection * conn, uint16_t port){
    err_t err;
    tcp_pcb* pcb;

    pcb = tcp_new();
    if(!pcb){ return false; }

    err = tcp_bind(pcb, HAPLWIP_BIND_ADDR, port);
    if(err != ERR_OK){
        HAP_DEBUG("[lwip] Failed to bind pcb\n");
        tcp_close(pcb);
        return false;
    }

    conn->raw = tcp_listen(pcb);
    if(!conn->raw){
        HAP_DEBUG("[lwip] Failed to listen\n");
        tcp_close(pcb);
        return false;
    }

    tcp_arg(HAPCONN_PCB(conn), conn);
    tcp_accept(HAPCONN_PCB(conn), &hap_lwip_accept);

    return true;
}

/**
 * Send data to clients
 * @param client
 * @param data
 * @param length
 * @return
 */
bool hap_network_send(hap_network_connection * client, const uint8_t * data, unsigned int length){
    auto err = tcp_write(HAPCONN_PCB(client), data, static_cast<u16_t>(length), 0);
    if(err != ERR_OK){
        HAP_DEBUG("[lwip] Unable to write data to buffer: %ld\n", err);
        return false;
    }

    err = tcp_output(HAPCONN_PCB(client));
    if(err != ERR_OK){
        HAP_DEBUG("[lwip] Unable to send packet: %ld\n", err);
        return false;
    }

    return false;
}

/**
 * Close tcp connection and call the callbacks
 * @param client
 */
void hap_network_close(hap_network_connection * client){
    hap_event_network_close(client);
    _hap_lwip_close(HAPCONN_PCB(client));
    delete client; //At last, we have to delete the memory allocated for the connection
}

#endif
