//
// Created by Xule Zhou on 7/29/18.
//

#ifndef HAPD_HAPPERSISTENTSTORAGE_H
#define HAPD_HAPPERSISTENTSTORAGE_H

#include "common.h"

/**
 * The 4bytes flags
 */
struct PersistFlags {
    /**
     * This byte stores information about cryptography keys
     *
     * For fixed blocks:
     * > 0b00000001 = ed25519 keys generated
     *
     * For dynamic blocks:
     * > todo...
     */
    uint8_t cryptography;

    uint8_t b, c, d;
};

/**
 * Dynamic block for paired devices
 */
struct PairedDevice {
    uint8_t identifier[36];
    uint8_t publicKey[32];
    PersistFlags flags;
};

/**
 * A wrapper class for c persistent functions
 */
class HAPPersistingStorage {
public:
    HAPPersistingStorage();
    ~HAPPersistingStorage();

    /**
     * Check if AccessoryLTPK and Accessory LTSK are present in
     * persistent storage.
     *
     * @return true if exists
     */
    bool haveAccessoryLongTermKeys();

    /**
     * Persistently stores the AccessoryLTPK and Accessory LTSK
     *
     * @param publicKey 32 bytes AccessoryLTPK
     * @param privateKey 32 bytes AccessoryLTSK
     */
    void setAccessoryLongTermKeys(const uint8_t * publicKey, const uint8_t * privateKey);

    /**
     * Read saved AccessoryLTPK and Accessory LTSK
     *
     * @param publicKey 32 bytes allocated buffer for AccessoryLTPK
     * @param privateKey 32 bytes allocated buffer for AccessoryLTSK
     */
    void getAccessoryLongTermKeys(uint8_t * publicKey, uint8_t * privateKey);

    /**
     * Read saved AccessoryLTPK
     *
     * @param publicKey 32 bytes allocated buffer
     */
    void getAccessoryLTPK(uint8_t * publicKey);

    /**
     * Store the paired device persistently
     *
     * @param identifier 36 bytes device pairing identifier
     * @param publicKey 32 bytes device public key
     * @param flags additional flags for permissions and others
     */
    void addPairedDevice(const uint8_t * identifier, const uint8_t * publicKey, const PersistFlags * flags = nullptr);

    /**
     * Remove a paired device from storage
     *
     * @return
     */
    bool removePairedDevice(const uint8_t * identifier);

    /**
     * Retrieve the stored device pairing data
     *
     * @param identifier 36 bytes device identifier
     */
    PairedDevice * retrievePairedDevice(const uint8_t * identifier);

    /**
     * Remove and wipe all the data from persist storage
     */
    void format();

    /**
     * @return Number of paired devices
     */
    unsigned int pairedDevicesCount();

    /**
     * Set the number of paired devices
     */
    void setPairedDeviceCount(unsigned int);

private:
    void * handle;
    PersistFlags * flags;

    void writeFlags();
};

#endif //HAPD_HAPPERSISTENTSTORAGE_H
