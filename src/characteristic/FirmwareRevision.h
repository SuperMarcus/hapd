//
// Created by Xule Zhou on 8/7/18.
//

#ifndef HAPD_FIRMWAREREVISION_H
#define HAPD_FIRMWAREREVISION_H

#include "GenericCharacteristic.h"

class FirmwareRevision : public GenericCharacteristic<0x00000052, CPERM_PR, FORMAT_STRING, const char *> {
public:
    explicit FirmwareRevision(unsigned int parentAccessory, HAPServer *server) : GenericCharacteristic(parentAccessory, server) {
        value.string_value = "1.0.0";
    }
};

#endif //HAPD_FIRMWAREREVISION_H
