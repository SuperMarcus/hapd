//
// Created by Xule Zhou on 8/7/18.
//

#ifndef ARDUINOHOMEKIT_SERIALNUMBER_H
#define ARDUINOHOMEKIT_SERIALNUMBER_H

#include "GenericCharacteristic.h"

class SerialNumber : public GenericCharacteristic<0x00000030, CPERM_PR, FORMAT_STRING, const char *> {
public:
    explicit SerialNumber(HAPServer *server) : GenericCharacteristic(server) {
        value.string_value = "000000000";
    }
};

#endif //ARDUINOHOMEKIT_SERIALNUMBER_H
