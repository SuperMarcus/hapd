//
// Created by Xule Zhou on 7/29/18.
//

#ifndef HAPD_PERSISTENCE_H
#define HAPD_PERSISTENCE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Persistence Storage Blocks
 *
 * Version 0x02: Pad all sizes to multiples of 32bits
 *
 * There are two sections in the persistence storage data structure:
 * - A fixed section of 74 bytes
 * - A dynamic section consisting of 72bytes blocks
 *
 *  -------------------
 * |   Fixed Section   |
 *  -------------------
 * 0x00       | ---
 *  > 4byte   | Storage version = 0x02
 * 0x04       | ---
 *  > 4bytes  | Flags
 * 0x08       | ---
 *  > 32bytes | AccessoryLTPK: ed25519 public key
 * 0x28       | ---
 *  > 64bytes | AccessoryLTSK: ed25519 private key
 * 0x68       | ---
 *  > 4bytes  | Number of objects in dynamic section in big endian
 * 0x6c       | ---
 *  -----------------------------
 * |  Dynamic Block (One Block)  |
 *  -----------------------------
 * 0x00       | ---
 *  > 36bytes | iOSDevicePairingID
 * 0x24       | ---
 *  > 32bytes | iOSDeviceLTPK
 * 0x44       | ---
 *  > 4bytes  | Flags
 * 0x48       | ---
 */

#define HAP_FIXED_BLOCK_SIZE    0x6c
#define HAP_DYNAM_BLOCK_SIZE    0x48
#define HAP_STORAGE_FMT_VERSION 0x02

/**
 * Allocate and initialize the persistence handle
 *
 * @return handle of the persistence structure
 */
void *hap_persistence_init();

/**
 * Read from a particular address of given length to the given buffer
 *
 * @param address Address to read from
 * @param buffer Buffer to store the read data
 * @param length Length of data to read
 * @return true if succeed
 */
bool hap_persistence_read(void *, unsigned int address, uint8_t *buffer, unsigned int length);

/**
 * Write to a particular address
 *
 * @param address Address to write the buffer to
 * @param buffer Data
 * @param length Length of data to write
 * @return true if succeed
 */
bool hap_persistence_write(void *, unsigned int address, const uint8_t *buffer, unsigned int length);

/**
 * Allocate and format the fixed section to zeros,
 * deleting all data stored in dynamic section
 */
void hap_persistence_format(void *);

/**
 * Close and deallocate the persistence handle
 *
 * @return
 */
void hap_persistence_deinit(void *);

#ifdef __cplusplus
}
#endif

#endif //HAPD_PERSISTENCE_H
