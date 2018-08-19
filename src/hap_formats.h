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

#ifndef HAPD_HAP_FORMATS_H
#define HAPD_HAP_FORMATS_H

union CharacteristicValue {
    const char * string_value;
    bool bool_value;
    int32_t int_value;
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

#define CPERM_PR    0b00000001
#define CPERM_PW    0b00000010
#define CPERM_EV    0b00000100
#define CPERM_AA    0b00001000
#define CPERM_TW    0b00010000
#define CPERM_HD    0b00100000

#define SRV_PRIMARY 0b00000001
#define SRV_HIDDEN  0b00000010

typedef uint8_t CharacteristicPermissions;

#endif //HAPD_HAP_FORMATS_H
