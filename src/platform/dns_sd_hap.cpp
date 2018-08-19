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

#ifdef USE_APPLE_DNS_SD

#include <dns_sd.h>
#include <cstring>
#include "../network.h"

void * hap_service_discovery_init(const char * name, uint16_t port){
    auto ref = DNSServiceRef();
    auto ret = DNSServiceRegister(
            &ref, 0, 0, name, "_hap._tcp",
            nullptr, nullptr, htons(port),
            0, nullptr, nullptr, nullptr
    );
    if(ret == kDNSServiceErr_NoError) return ref;
    HAP_DEBUG("DNS Service register failure: %d", ret);
    return nullptr;
}

void hap_service_discovery_loop(void *){}

bool hap_service_discovery_update(void * handle, hap_sd_txt_item * records){
    auto ref = reinterpret_cast<DNSServiceRef>(handle);
    auto bufLen = 0;
    auto current = records;
    while (current->key != nullptr){
        bufLen += strlen(current->key) + strlen(current->value) + 2;
        current++;
    }
    auto buf = new char[bufLen];
    current = records;
    auto bufPtr = buf;
    while (current->key != nullptr){
        auto kLen = strlen(current->key);
        auto vLen = strlen(current->value);
        *bufPtr++ = static_cast<char>(kLen + vLen + 1);
        memcpy(bufPtr, current->key, kLen);
        bufPtr += kLen;
        *bufPtr++ = '=';
        memcpy(bufPtr, current->value, vLen);
        bufPtr += vLen;
        current++;
    }
    auto ret = DNSServiceUpdateRecord(ref, nullptr, 0, static_cast<uint16_t>(bufLen), buf, 0);
    delete[] buf;
    if(ret == kDNSServiceErr_NoError) return true;
    HAP_DEBUG("Unable to update sd records: %d", ret);
    return false;
}

void hap_service_discovery_deinit(void * handle){
    auto ref = reinterpret_cast<DNSServiceRef>(handle);
    DNSServiceRefDeallocate(ref);
}

#endif
