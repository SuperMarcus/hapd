//
// Created by Xule Zhou on 8/7/18.
//

#ifndef ARDUINOHOMEKIT_IDENTITY_H
#define ARDUINOHOMEKIT_IDENTITY_H

#include "GenericCharacteristic.h"

class Identity: public GenericCharacteristic<0x00000014, CPERM_PW, FORMAT_BOOL, bool> {
public:
    Identity(unsigned int cid, HAPServer *server) : GenericCharacteristic(cid, server) {
        value.bool_value = false;
    }

public:

};

#endif //ARDUINOHOMEKIT_IDENTITY_H
