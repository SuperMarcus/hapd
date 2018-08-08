//
// Created by Xule Zhou on 8/7/18.
//

#ifndef ARDUINOHOMEKIT_ON_H
#define ARDUINOHOMEKIT_ON_H

#include "GenericCharacteristic.h"

class On : public GenericCharacteristic<0x00000025, CPERM_PW | CPERM_PR | CPERM_EV, FORMAT_BOOL, bool> {
public:
    explicit On(unsigned int parentAccessory, HAPServer *server) : GenericCharacteristic(parentAccessory, server) { }
};

#endif //ARDUINOHOMEKIT_ON_H
