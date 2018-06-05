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
    uint8_t offset;

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
 * Find the item with the specific type in the chain. This function will
 * search the item from the start of the chain, given that the chain passed
 * can be at any position in the chain.
 *
 * @param chain
 * @param type
 * @return
 */
tlv8_item * tlv8_find(tlv8_item * chain, tlv8_type type);

/**
 * Read 'length' bytes of data from 'item' to 'buffer'. This will shift the
 * offset of the items in chain.
 *
 * @param item Tlv8 item to be read from
 * @param buffer Destination buffer
 * @param length Number of bytes to be read
 * @return Actual number of bytes copied to buffer
 */
unsigned int tlv8_read(tlv8_item * item, uint8_t * buffer, unsigned int length);

/**
 * Free up the memory allocated to the parsed tlv8_item * chain
 *
 * @param chain
 */
void tlv8_free(tlv8_item * chain);

#endif //ARDUINOHOMEKIT_TLV_H
