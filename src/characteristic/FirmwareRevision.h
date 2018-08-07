//
// Created by Xule Zhou on 8/7/18.
//

#ifndef ARDUINOHOMEKIT_FIRMWAREREVISION_H
#define ARDUINOHOMEKIT_FIRMWAREREVISION_H

#include "GenericCharacteristic.h"

class FirmwareRevision : public GenericCharacteristic<0x00000052, CPERM_PR, FORMAT_STRING, const char *> {
public:
    FirmwareRevision(unsigned int cid, HAPServer *server) : GenericCharacteristic(cid, server) {
        value.string_value = "1.0.0";
    }
};

#endif //ARDUINOHOMEKIT_FIRMWAREREVISION_H
