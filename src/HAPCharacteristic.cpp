#include "HomeKitAccessory.h"

static void _freeHelper(HAPEvent * event){
    auto c = event->arg<BaseCharacteristic>();
    c->lastOperator->release();
    c->lastOperator = nullptr;
}

BaseCharacteristic::BaseCharacteristic(unsigned int parentAccessory, HAPServer *server, uint32_t type):
    server(server), characteristicTypeIdentifier(type), accessoryIdentifier(parentAccessory){
    instanceIdentifier = server->instanceIdPool++;
}

void BaseCharacteristic::setValue(CharacteristicValue v, HAPUserHelper * sender) {
    lastOperator = sender;
    value = v;
    if(sender) sender->retain();
    server->emit(HAPEvent::HAP_CHARACTERISTIC_UPDATE, this, &_freeHelper);
}
