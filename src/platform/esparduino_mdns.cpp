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
