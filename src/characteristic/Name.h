//
// Created by Xule Zhou on 8/7/18.
//

#ifndef ARDUINOHOMEKIT_NAME_H
#define ARDUINOHOMEKIT_NAME_H

#include "GenericCharacteristic.h"

class Name : public GenericCharacteristic<0x00000023, CPERM_PR, FORMAT_STRING, const char *>{
public:
    Name(unsigned int cid, HAPServer *server) : GenericCharacteristic(cid, server) {
        value.string_value = server->deviceName;
    }

};

#endif //ARDUINOHOMEKIT_NAME_H
