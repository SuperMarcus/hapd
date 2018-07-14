#include "../common.h"

#ifdef USE_APPLE_DNS_SD

#include <dns_sd.h>
#include <cstring>
#include "../network.h"

void * hap_service_discovery_init(const char * name, uint16_t port){
    auto ref = new DNSServiceRef();
    auto ret = DNSServiceRegister(
            ref, 0, 0, name, "_hap._tcp",
            nullptr, nullptr, htons(port),
            0, nullptr, nullptr, nullptr
    );
    if(ret == kDNSServiceErr_NoError) return ref;
    HAP_DEBUG("DNS Service register failure: %d", ret);
    return nullptr;
}

bool hap_service_discovery_update(void * handle, hap_sd_txt_item * records){
    auto ref = reinterpret_cast<DNSServiceRef *>(handle);
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
    auto ret = DNSServiceUpdateRecord(*ref, nullptr, 0, static_cast<uint16_t>(bufLen), buf, 0);
    if(ret == kDNSServiceErr_NoError) return true;
    HAP_DEBUG("Unable to update sd records: %d", ret);
    return false;
}

#endif
