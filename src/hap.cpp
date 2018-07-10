#include "HAPServer.h"
#include "tlv.h"

HAPServer HomeKitAccessory;

void HAPServer::_onRequestReceived(hap_network_connection * conn) {
    HAP_DEBUG("new message received: %s, length: %u", conn->user->request_buffer, conn->user->request_current_length);
}

void HAPServer::_s_onRequestReceived(hap_network_connection *conn, void *arg) {
    static_cast<HAPServer *>(arg)->_onRequestReceived(conn);
}

void HAPServer::begin(uint16_t port) {
    server_conn = new hap_network_connection;
    server_conn->raw = nullptr;
    server_conn->user = nullptr;
    server_conn->request_cb_arg = this;
    server_conn->request_cb = &HAPServer::_s_onRequestReceived;

    hap_network_init_bind(server_conn, port);
}
