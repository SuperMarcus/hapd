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

#include "HomeKitAccessory.h"
#include "service/AccessoryInformation.h"

BaseAccessory::BaseAccessory(unsigned int aid, HAPServer * s):
        accessoryIdentifier(aid), server(s) {
    addService<AccessoryInformation>();
}

void BaseAccessory::_addService(BaseService * s) {
    auto current = &services;
    while (*current != nullptr){
        current = &(*current)->next;
    }
    *current = s;
}

BaseCharacteristic *BaseAccessory::getCharacteristic(unsigned int iid) {
    BaseCharacteristic * res = nullptr;
    auto currentAcc = services;
    while (currentAcc != nullptr && res == nullptr){
        auto currentChar = currentAcc->characteristics;
        while (currentChar != nullptr && res == nullptr){
            if(currentChar->instanceIdentifier == iid) res = currentChar;
            currentChar = currentChar->next;
        }
        currentAcc = currentAcc->next;
    }
    return res;
}
