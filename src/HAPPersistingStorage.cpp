//
// Created by Xule Zhou on 7/29/18.
//

#include <cctype>

#include "HAPPersistingStorage.h"

#include "common.h"
#include "persistence.h"

#define FIXED_STORVER_ADDR  0x00
#define FIXED_STORVER_LEN   1
#define FIXED_FLAGS_ADDR    0x01
#define FIXED_FLAGS_LEN     4
#define FIXED_LTPK_ADDR     0x05
#define FIXED_LTPK_LEN      32
#define FIXED_LTSK_ADDR     0x25
#define FIXED_LTSK_LEN      32
#define FIXED_OBJCNT_ADDR   0x45
#define FIXED_OBJCNT_LEN    2

#define FLAG_CRYPTO_KEYS    0b00000001

HAPPersistingStorage::HAPPersistingStorage() {
    handle = hap_persistence_init();
    flags = new PersistFlags();

    uint8_t storageVersion = 0;
    hap_persistence_read(handle, FIXED_STORVER_ADDR, &storageVersion, FIXED_STORVER_LEN);

    if(storageVersion != HAP_STORAGE_FMT_VERSION){ format(); }

    hap_persistence_read(handle, FIXED_FLAGS_ADDR, reinterpret_cast<uint8_t *>(flags), FIXED_FLAGS_LEN);
}

HAPPersistingStorage::~HAPPersistingStorage() {
    hap_persistence_deinit(handle);
    delete flags;
}

void HAPPersistingStorage::format() {
    hap_persistence_format(handle);

    //Write version number
    uint8_t ver = HAP_STORAGE_FMT_VERSION;
    hap_persistence_write(handle, FIXED_STORVER_ADDR, &ver, FIXED_STORVER_LEN);
}

bool HAPPersistingStorage::haveAccessoryLongTermKeys() {
    return ((flags->cryptography) & FLAG_CRYPTO_KEYS) == FLAG_CRYPTO_KEYS; // NOLINT
}

void HAPPersistingStorage::setAccessoryLongTermKeys(uint8_t *publicKey, uint8_t *privateKey) {
    auto ret = hap_persistence_write(handle, FIXED_LTPK_ADDR, publicKey, FIXED_LTPK_LEN);
    ret &= hap_persistence_write(handle, FIXED_LTSK_ADDR, privateKey, FIXED_LTSK_LEN);

    if(ret){
        flags->cryptography |= FLAG_CRYPTO_KEYS;
        writeFlags();
    }
}

void HAPPersistingStorage::writeFlags() {
    if(!hap_persistence_write(handle, FIXED_FLAGS_ADDR, reinterpret_cast<uint8_t *>(flags), FIXED_FLAGS_LEN)){
        HAP_DEBUG("Unable to update persist flags");
    }
}

void HAPPersistingStorage::getAccessoryLongTermKeys(uint8_t *publicKey, uint8_t *privateKey) {
    hap_persistence_read(handle, FIXED_LTPK_ADDR, publicKey, FIXED_LTPK_LEN);
    hap_persistence_read(handle, FIXED_LTSK_ADDR, privateKey, FIXED_LTSK_LEN);
}
