#include "HAPServer.h"
#include "tlv.h"

HAPServer::HAPServer() {

}

void HAPServer::_onRequestReceived(hap_network_connection *conn) {

}

void HAPServer::_s_onRequestReceived(hap_network_connection *conn, void *arg) {
    static_cast<HAPServer *>(arg)->_onRequestReceived(conn);
}
