#include "HomeKitAccessory.h"
#include "service/AccessoryInformation.h"

BaseAccessory::BaseAccessory(unsigned int aid, HAPServer * s):
        accessoryIdentifier(aid), server(s) {
    addService<AccessoryInformation>();
}

void BaseAccessory::_addService(BaseService * s) {
    auto current = &services;
    while (*current != nullptr){
        current = &(*current)->next;
    }
    *current = s;
}
