#include "HomeKitAccessory.h"

BaseCharacteristic::BaseCharacteristic(HAPServer *server, uint32_t type):
    server(server), characteristicTypeIdentifier(type){
    instanceIdentifier = server->instanceIdPool++;
}

void BaseCharacteristic::setValue(CharacteristicValue v) {
    value = v;
}
