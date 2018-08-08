#include "HomeKitAccessory.h"

BaseService::BaseService(unsigned int parentAccessory, uint32_t type, HAPServer * s):
        serviceTypeIdentifier(type), server(s), accessoryIdentifier(parentAccessory) {
    instanceIdentifier = server->instanceIdPool++;
}

void BaseService::addCharacteristic(BaseCharacteristic * c) {
    auto current = &characteristics;
    while (*current != nullptr){
        current = &(*current)->next;
    }
    *current = c;
}
