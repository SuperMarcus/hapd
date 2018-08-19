//
// Created by Xule Zhou on 8/7/18.
//

#ifndef HAPD_GENERICSERVICE_H
#define HAPD_GENERICSERVICE_H

template <uint32_t UUID, uint8_t Perm, CharacteristicValueFormat Fmt, typename ResT>
class GenericCharacteristic: public BaseCharacteristic {
public:
    explicit GenericCharacteristic(unsigned int parentAccessory, HAPServer *server) :
            BaseCharacteristic(parentAccessory, server, static_cast<uint32_t>(UUID)) {
        format = Fmt;
        value.uint8_value = 0;
        permissions = Perm;
    }

    ResT get(){ return static_cast<ResT>(value); }

    void set(ResT v){ setValue(static_cast<CharacteristicValue>(v)); }

    SCONST uint32_t type = static_cast<uint32_t>(UUID);
};

#endif //HAPD_GENERICSERVICE_H
