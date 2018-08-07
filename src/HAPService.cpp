#include "HomeKitAccessory.h"

BaseService::BaseService(unsigned int sid, uint32_t type, HAPServer * s):
        serviceIdentifier(sid), serviceTypeIdentifier(type), server(s) {

}

void BaseService::addCharacteristic(BaseCharacteristic * c) {
    auto current = &characteristics;
    while (*current != nullptr){
        current = &(*current)->next;
    }
    *current = c;
}
