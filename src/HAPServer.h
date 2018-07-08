#include "network.h"
#include "tlv.h"

class HAPServer {
public:
    HAPServer();//NOLINT

private:
    friend void hap_event_network_receive(hap_network_connection *, const uint8_t *, unsigned int);
    static void _s_onRequestReceived(hap_network_connection * conn, void * arg);

    void _onRequestReceived(hap_network_connection * conn);
};

extern HAPServer HomeKit;
