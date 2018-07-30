//
// Created by Xule Zhou on 7/29/18.
//

#ifndef ARDUINOHOMEKIT_HAPPERSISTENTSTORAGE_H
#define ARDUINOHOMEKIT_HAPPERSISTENTSTORAGE_H

#include "common.h"

/**
 * The 4bytes flags
 */
struct PersistFlags {
    /**
     * This byte stores information about cryptography keys
     *
     * 0b00000001 = ed25519 keys generated
     */
    uint8_t cryptography;

    uint8_t b, c, d;
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
    void setAccessoryLongTermKeys(uint8_t * publicKey, uint8_t * privateKey);

    /**
     * Read saved AccessoryLTPK and Accessory LTSK
     *
     * @param publicKey 32 bytes allocated buffer for AccessoryLTPK
     * @param privateKey 32 bytes allocated buffer for AccessoryLTSK
     */
    void getAccessoryLongTermKeys(uint8_t * publicKey, uint8_t * privateKey);

    /**
     * Remove and wipe all the data from persist storage
     */
    void format();

private:
    void * handle;
    PersistFlags * flags;

    void writeFlags();
};

#endif //ARDUINOHOMEKIT_HAPPERSISTENTSTORAGE_H
