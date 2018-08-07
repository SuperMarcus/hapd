//
// Created by Xule Zhou on 8/7/18.
//

#ifndef ARDUINOHOMEKIT_SWITCH_H
#define ARDUINOHOMEKIT_SWITCH_H

#include "GenericService.h"
#include "../characteristic/On.h"

class Switch : public GenericService<0x00000049> {
public:
    explicit Switch(HAPServer *s) : GenericService(s) {
        addCharacteristic<On>();
    }
};

#endif //ARDUINOHOMEKIT_SWITCH_H
