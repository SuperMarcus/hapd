//
// Created by Xule Zhou on 8/7/18.
//

#ifndef ARDUINOHOMEKIT_MODEL_H
#define ARDUINOHOMEKIT_MODEL_H

#include "GenericCharacteristic.h"

class Model: public GenericCharacteristic<0x00000021, CPERM_PR, FORMAT_STRING, const char *>{
public:
    explicit Model(HAPServer *server) : GenericCharacteristic(server) {
        value.string_value = server->modelName;
    }
};

#endif //ARDUINOHOMEKIT_MODEL_H