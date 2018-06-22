#include <Arduino.h>

#include "network.h"
#include "tlv.h"

class HomeKit {
private:
    HomeKit();//NOLINT

private:
    friend void hap_event_network_receive(hap_network_connection *, const uint8_t *, unsigned int);
    void _onRequestReceived(hap_network_connection * conn);

public:
    static HomeKit shared = HomeKit();
};
