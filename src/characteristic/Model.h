//
// Created by Xule Zhou on 8/7/18.
//

#ifndef HAPD_MODEL_H
#define HAPD_MODEL_H

#include "GenericCharacteristic.h"

class Model: public GenericCharacteristic<0x00000021, CPERM_PR, FORMAT_STRING, const char *>{
public:
    explicit Model(unsigned int parentAccessory, HAPServer *server) : GenericCharacteristic(parentAccessory, server) {
        value.string_value = server->modelName;
    }
};

#endif //HAPD_MODEL_H
