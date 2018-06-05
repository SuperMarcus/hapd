//
// Created by Xule Zhou on 6/4/18.
//

#ifndef ARDUINOHOMEKIT_TLV_H
#define ARDUINOHOMEKIT_TLV_H

#include "common.h"

enum tlv8_error_code: uint8_t {
    kTLVError_Unknown           = 0x01,
    kTLVError_Authentication    = 0x02,
    kTLVError_Backoff           = 0x03,
    kTLVError_MaxPeers          = 0x04,
    kTLVError_MaxTries          = 0x05,
    kTLVError_Unavailable       = 0x06,
    kTLVError_Busy              = 0x07
};

enum tlv8_type: uint8_t {
    kTLVType_Method             = 0x00,
    kTLVType_Identifier         = 0x01,
    kTLVType_Salt               = 0x02,
    kTLVType_PublicKey          = 0x03,
    kTLVType_Proof              = 0x04,
    kTLVType_EncryptedData      = 0x05,
    kTLVType_State              = 0x06,
    kTLVType_Error              = 0x07,
    kTLVType_RetryDelay         = 0x08,
    kTLVType_Certificate        = 0x09,
    kTLVType_Signature          = 0x0a,
    kTLVType_Permissions        = 0x0b,
    kTLVType_FragmentData       = 0x0c,
    kTLVType_FragmentLast       = 0x0d,
    kTLVType_Separator          = 0xff
};

typedef uint8_t tlv8_length;

struct tlv8_item {
    tlv8_type type;
    tlv8_length length;
    uint8_t * value;

    tlv8_item * previous;
    tlv8_item * next;
};

/**
 * Parse the tlv8 formatted data and returns the tlv8_item chain
 *
 * @param data Data to be parsed
 * @param length Length of the data
 * @return nullptr if unsuccessful, a pointer otherwise
 */
tlv8_item * tlv8_parse(uint8_t * data, unsigned int length);

/**
 * Free up the memory allocated to the parsed tlv8_item * chain
 *
 * @param chain
 */
void tlv8_free(tlv8_item * chain);

#endif //ARDUINOHOMEKIT_TLV_H
