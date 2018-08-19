//
// Created by Xule Zhou on 8/7/18.
//

#ifndef HAPD_MANUFACTURER_H
#define HAPD_MANUFACTURER_H

#include "GenericCharacteristic.h"

class Manufacturer: public GenericCharacteristic<0x00000020, CPERM_PR, FORMAT_STRING, const char *>{
public:
    explicit Manufacturer(unsigned int parentAccessory, HAPServer *server) : GenericCharacteristic(parentAccessory, server) {
        value.string_value = "HomeKitManufacturer";
    }
};

#endif //HAPD_MANUFACTURER_H
