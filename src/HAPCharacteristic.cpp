#include "HomeKitAccessory.h"

BaseCharacteristic::BaseCharacteristic(unsigned int cid, HAPServer *server, uint32_t type):
    characteristicIdentifier(cid), server(server), characteristicTypeIdentifier(type){

}

void BaseCharacteristic::setValue(CharacteristicValue v) {
    value = v;
}
