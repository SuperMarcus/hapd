//
// Created by Xule Zhou on 7/29/18.
//

#include <cctype>
#include <cstring>

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

#define DYNAM_PAIR_ID_ADDR  0x00
#define DYNAM_PAIR_ID_LEN   36
#define DYNAM_LTPK_ADDR     0x24
#define DYNAM_LTPK_LEN      32
#define DYNAM_FLAGS_ADDR    0x44
#define DYNAM_FLAGS_LEN     4

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
    HAP_DEBUG("Formatting persist storage...");

    hap_persistence_format(handle);

    //Write version number
    uint8_t ver = HAP_STORAGE_FMT_VERSION;
    hap_persistence_write(handle, FIXED_STORVER_ADDR, &ver, FIXED_STORVER_LEN);
}

bool HAPPersistingStorage::haveAccessoryLongTermKeys() {
    return ((flags->cryptography) & FLAG_CRYPTO_KEYS) == FLAG_CRYPTO_KEYS; // NOLINT
}

void HAPPersistingStorage::setAccessoryLongTermKeys(const uint8_t *publicKey, const uint8_t *privateKey) {
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

void HAPPersistingStorage::getAccessoryLTPK(uint8_t *publicKey) {
    hap_persistence_read(handle, FIXED_LTPK_ADDR, publicKey, FIXED_LTPK_LEN);
}

void HAPPersistingStorage::addPairedDevice(const uint8_t *identifier, const uint8_t *publicKey, const PersistFlags *flags) {
    auto previousCount = pairedDevicesCount();
    auto device = PairedDevice();

    memcpy(device.identifier, identifier, 36);
    memcpy(device.publicKey, publicKey, 32);

    if(flags) memcpy(&device.flags, flags, 4);
    else memset(&device.flags, 0, 4);

    hap_persistence_write(
            handle,
            HAP_FIXED_BLOCK_SIZE + (previousCount * HAP_DYNAM_BLOCK_SIZE),
            reinterpret_cast<uint8_t *>(&device),
            HAP_DYNAM_BLOCK_SIZE
    );
    setPairedDeviceCount(previousCount + 1);

    //todo
    HAP_DEBUG("Device persistently saved.");
    extern void hexdump(const void *ptr, int buflen);
    hexdump(device.identifier, 36);
    hexdump(device.publicKey, 32);
}

unsigned int HAPPersistingStorage::pairedDevicesCount() {
    uint8_t buf[FIXED_OBJCNT_LEN];
    unsigned int cnt = 0;
    hap_persistence_read(handle, FIXED_OBJCNT_ADDR, buf, FIXED_OBJCNT_LEN);

    for (const auto i : buf) {
        cnt *= 0xff;
        cnt += i;
    }

    return cnt;
}

void HAPPersistingStorage::setPairedDeviceCount(unsigned int cnt) {
    uint8_t buf[FIXED_OBJCNT_LEN];
    for(auto& b : buf){
        b = static_cast<uint8_t>(cnt % 0xff);
        cnt /= 0xff;
    }
    hap_persistence_write(handle, FIXED_OBJCNT_ADDR, buf, FIXED_OBJCNT_LEN);
}

PairedDevice * HAPPersistingStorage::retrievePairedDevice(const uint8_t *identifier) {
    int i = pairedDevicesCount();
    uint8_t idBuf[DYNAM_PAIR_ID_LEN];
    while (--i >= 0){
        auto addr = static_cast<unsigned int>(HAP_FIXED_BLOCK_SIZE + (i * HAP_DYNAM_BLOCK_SIZE));
        hap_persistence_read(
                handle,
                addr + DYNAM_PAIR_ID_ADDR,
                idBuf,
                DYNAM_PAIR_ID_LEN
        );
        if(memcmp(idBuf, identifier, DYNAM_PAIR_ID_LEN) == 0){
            auto device = new PairedDevice;
            hap_persistence_read(handle, addr, reinterpret_cast<uint8_t *>(device), HAP_DYNAM_BLOCK_SIZE);
            return device;
        }
    }
    return nullptr;
}
