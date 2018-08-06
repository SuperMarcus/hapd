//
// Created by Xule Zhou on 8/6/18.
//

#ifndef ARDUINOHOMEKIT_HAP_FORMATS_H
#define ARDUINOHOMEKIT_HAP_FORMATS_H

union CharacteristicValue {
    char * string_value;
    bool bool_value;
    uint8_t uint8_value;
    uint16_t uint16_value;
    uint32_t uint32_value;
    uint64_t uint64_value;
    double float_value;
};

enum CharacteristicValueFormat {
    FORMAT_STRING = 0,
    FORMAT_BOOL,
    FORMAT_UINT8,
    FORMAT_UINT16,
    FORMAT_UINT32,
    FORMAT_UINT64,
    FORMAT_INT,
    FORMAT_FLOAT,
    FORMAT_TLV8,
    FORMAT_DATA
};

#endif //ARDUINOHOMEKIT_HAP_FORMATS_H
