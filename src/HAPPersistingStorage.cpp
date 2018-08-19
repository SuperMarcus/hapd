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

#include <cctype>
#include <cstring>

#include "HAPPersistingStorage.h"

#include "common.h"
#include "persistence.h"

#define FIXED_STORVER_ADDR  0x00
#define FIXED_STORVER_LEN   4
#define FIXED_FLAGS_ADDR    0x04
#define FIXED_FLAGS_LEN     4
#define FIXED_LTPK_ADDR     0x08
#define FIXED_LTPK_LEN      32
#define FIXED_LTSK_ADDR     0x28
#define FIXED_LTSK_LEN      64
#define FIXED_OBJCNT_ADDR   0x68
#define FIXED_OBJCNT_LEN    4

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

    uint32_t storageVersion = 0;
    hap_persistence_read(handle, FIXED_STORVER_ADDR, reinterpret_cast<uint8_t *>(&storageVersion), FIXED_STORVER_LEN);

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
    for(auto i = FIXED_OBJCNT_LEN - 1; i >= 0; --i){
        buf[i] = static_cast<uint8_t>(cnt % 0xff);
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

bool HAPPersistingStorage::removePairedDevice(const uint8_t *identifier) {
    auto origCnt = pairedDevicesCount();
    auto cnt = origCnt;
    uint8_t idBuf[DYNAM_PAIR_ID_LEN];
    for(unsigned int i = 0; i < cnt; ++i){
        auto addr = HAP_FIXED_BLOCK_SIZE + (i * HAP_DYNAM_BLOCK_SIZE);
        hap_persistence_read(
                handle,
                addr + DYNAM_PAIR_ID_ADDR,
                idBuf,
                DYNAM_PAIR_ID_LEN
        );
        if(memcmp(idBuf, identifier, DYNAM_PAIR_ID_LEN) == 0){
            PairedDevice buf {};
            //Move the last block to the current block, and decrease the cnt value
            auto lastAddr = HAP_FIXED_BLOCK_SIZE + ((cnt - 1) * HAP_DYNAM_BLOCK_SIZE);
            if(lastAddr != addr){
                hap_persistence_read(handle, lastAddr, reinterpret_cast<uint8_t *>(&buf), HAP_DYNAM_BLOCK_SIZE);
                hap_persistence_write(handle, addr, reinterpret_cast<const uint8_t *>(&buf), HAP_DYNAM_BLOCK_SIZE);
            }
            //Write zeros to the last block
            memset(&buf, 0, HAP_DYNAM_BLOCK_SIZE);
            hap_persistence_write(handle, lastAddr, reinterpret_cast<const uint8_t *>(&buf), HAP_DYNAM_BLOCK_SIZE);
            cnt -= 1;
        }
    }
    HAP_DEBUG("Removed %d devices", origCnt - cnt);
    setPairedDeviceCount(cnt);
    return cnt < origCnt;
}
