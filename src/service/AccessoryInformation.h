/**
 * hapd
 *
 * Copyright 2018 Xule Zhou
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef HAPD_ACCESSORYINFORMATION_H
#define HAPD_ACCESSORYINFORMATION_H

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

#endif //HAPD_ACCESSORYINFORMATION_H
