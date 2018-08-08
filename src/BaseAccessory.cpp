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

BaseCharacteristic *BaseAccessory::getCharacteristic(unsigned int iid) {
    BaseCharacteristic * res = nullptr;
    auto currentAcc = services;
    while (currentAcc != nullptr && res == nullptr){
        auto currentChar = currentAcc->characteristics;
        while (currentChar != nullptr && res == nullptr){
            if(currentChar->instanceIdentifier == iid) res = currentChar;
            currentChar = currentChar->next;
        }
        currentAcc = currentAcc->next;
    }
    return res;
}
