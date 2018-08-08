//
// Created by Xule Zhou on 8/7/18.
//

#ifndef ARDUINOHOMEKIT_GENERIC_H
#define ARDUINOHOMEKIT_GENERIC_H

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

#endif //ARDUINOHOMEKIT_GENERIC_H
