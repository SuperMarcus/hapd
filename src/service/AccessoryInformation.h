//
// Created by Xule Zhou on 8/7/18.
//

#ifndef ARDUINOHOMEKIT_ACCESSORYINFORMATION_H
#define ARDUINOHOMEKIT_ACCESSORYINFORMATION_H

#include "GenericService.h"
#include "../characteristic/Identity.h"
#include "../characteristic/Manufacturer.h"
#include "../characteristic/Model.h"
#include "../characteristic/Name.h"
#include "../characteristic/SerialNumber.h"
#include "../characteristic/FirmwareRevision.h"

class AccessoryInformation: public GenericService<0x0000003E> {
public:
    AccessoryInformation(unsigned int parentAccessory, HAPServer * s) : GenericService(parentAccessory, s) {
        addCharacteristic<Identity>();
        addCharacteristic<Manufacturer>();
        addCharacteristic<Model>();
        addCharacteristic<Name>();
        addCharacteristic<SerialNumber>();
        addCharacteristic<FirmwareRevision>();
    }
};

#endif //ARDUINOHOMEKIT_ACCESSORYINFORMATION_H
