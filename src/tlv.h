//
// Created by Xule Zhou on 6/4/18.
//

#ifndef ARDUINOHOMEKIT_TLV_H
#define ARDUINOHOMEKIT_TLV_H

#include <cstdint>

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

enum tlv8_method: uint8_t {
    METHOD_RESERVED                 = 0,
    METHOD_PAIR_SETUP               = 1,
    METHOD_PAIR_VERIFY              = 2,
    METHOD_ADD_PAIRING              = 3,
    METHOD_REMOVE_PAIRING           = 4,
    METHOD_LIST_PAIRINGS            = 5
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
const tlv8_item * tlv8_find(const tlv8_item * chain, tlv8_type type) __attribute((pure));

/**
 * Find the item with the specific type after the given item.
 *
 * @param chain
 * @param type
 * @return
 */
const tlv8_item * tlv8_find_next(const tlv8_item * chain, tlv8_type type) __attribute((pure));

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
 * Get the length of the value contained in this tlv8_item. Use this method
 * instead of directly accessing the 'length' property of tlv8_item to get
 * accurate readings of the length, since items might be fragmented.
 *
 * @param item The tlv8 item
 * @return Length of the item
 */
unsigned int tlv8_value_length(const tlv8_item * item) __attribute((pure));

/**
 * Presume the number of bytes of tlv8_item after encodings given the length of
 * the actual value. This function takes into account of the fragmentation part
 * of tlv8.
 *
 * @param value_length Number of bytes of the value to be encoded
 * @return
 */
unsigned int tlv8_item_length(unsigned int value_length) __attribute((pure));

/**
 * Get the total number of bytes of the chain after encoding. This function will
 * also starts from the beginning of the chain.
 *
 * @param chain Any item in the chain
 * @return Number of bytes required to contain this chain.
 */
unsigned int tlv8_chain_length(const tlv8_item * chain) __attribute((pure));

/**
 * Free up the memory allocated to the parsed tlv8_item * chain, starts with the
 * first item in the chain.
 *
 * @param chain
 */
void tlv8_free(tlv8_item * chain);

/**
 * Detach the item from the chain and free up its memories. Note this will not
 * take fragmentation into account.
 *
 * @param item The item to be detached
 */
void tlv8_detach(tlv8_item * item);

/**
 * Reset all the offsets in the chain to 0. Also starts from the start of the
 * chain.
 *
 * @param chain
 */
void tlv8_reset_chain(tlv8_item * chain);

/**
 * Insert an item to the chain. This function won't encode the chain yet, it
 * creates a tlv8_item that points to the original data. This function will also
 * take fragmentation into account.
 *
 * @param chain The chain that this item will be added to. Pass nullptr to create
 *              a new chain.
 * @param type tlv8_type of this item
 * @param length Number of bytes of this item
 * @param data Pointer to the actual data
 * @return The newly created tlv8_item
 */
tlv8_item * tlv8_insert(tlv8_item * chain, tlv8_type type, unsigned int length, const void * data);

/**
 * Encode the tlv8 chain to the destination buffer. Its your responsibility to make
 * sure that the buffer is larger than the value returned from tlv8_chain_length().
 *
 * @param chain
 * @param destination
 * @param length
 */
void tlv8_encode(const tlv8_item * chain, uint8_t * destination);

/**
 * Alloc the destination buffer and encode the entire tlv8 chain into the buffer,
 * then free the entire tlv8 chain.
 *
 * @param chain
 * @return pointer to the allocated buffer with encoded data
 * @warning remember to delete[] the buffer
 */
uint8_t * tlv8_export_free(tlv8_item * chain, unsigned int * exportedLength);

#endif //ARDUINOHOMEKIT_TLV_H
