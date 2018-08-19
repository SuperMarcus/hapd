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

#ifndef HAPD_GENERIC_H
#define HAPD_GENERIC_H

#include <utility>

template <uint32_t UUID>
class GenericService: public BaseService {
public:
    explicit GenericService(unsigned int parentAccessory, HAPServer * s): BaseService(parentAccessory, static_cast<uint32_t>(UUID), s) {}

    SCONST uint32_t type = static_cast<uint32_t>(UUID);

protected:
    template <typename T, typename ...Args>
    T * addCharacteristic(Args&&... args){
        auto c = new T(accessoryIdentifier, server, std::forward<Args>(args)...);
        BaseService::addCharacteristic(c);
        return c;
    }

    unsigned int aidPool = 0;
};

#endif //HAPD_GENERIC_H
