#include "../common.h"

#ifdef USE_ESPARDUINO_MDNS

#include "../network.h"
#include <ESP8266mDNS.h>
#include <cctype>

struct esparduino_mdns_handle {
    MDNSResponder * responder;

    esparduino_mdns_handle(){
        responder = new MDNSResponder();
    }

    ~esparduino_mdns_handle(){
        delete responder;
    }
};

void hap_service_discovery_loop(void * h){
    auto handle = static_cast<esparduino_mdns_handle *>(h);
    handle->responder->update();
}

void * hap_service_discovery_init(const char * name, uint16_t port){
    auto handle = new esparduino_mdns_handle();
    auto mdns = handle->responder;
    auto hostname = new char[strlen(name) + 1]();

    auto c = name;
    auto d = hostname;
    for(; (*c) != '\x00'; ++c, ++d){
        *d = static_cast<char>(tolower(*c));
        *d = (*d >= 'a' && *d <= 'z') || (*d >= '0' && *d <= '9') ? *d : '_';
    }

    mdns->setInstanceName(name);
    mdns->begin(hostname);
    delete[] hostname;

    mdns->addService("hap", "tcp", port);

    return handle;
}

bool hap_service_discovery_update(void * h, hap_sd_txt_item * items){
    auto handle = static_cast<esparduino_mdns_handle *>(h);

    while (items->key != nullptr){
        handle->responder->addServiceTxt("hap", "tcp", items->key, items->value);
        items++;
    }

    return true;
}

void hap_service_discovery_deinit(void * h){
    auto handle = static_cast<esparduino_mdns_handle *>(h);
    delete handle;
}

#endif
