#include "HomeKitAccessory.h"

BaseService::BaseService(uint32_t type, HAPServer * s):
        serviceTypeIdentifier(type), server(s) {
    instanceIdentifier = server->instanceIdPool++;
}

void BaseService::addCharacteristic(BaseCharacteristic * c) {
    auto current = &characteristics;
    while (*current != nullptr){
        current = &(*current)->next;
    }
    *current = c;
}
