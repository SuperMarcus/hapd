#include "../common.h"

#ifdef USE_SPIFLASH_PERSISTENT

#include <Arduino.h>
extern "C" {
#include <spi_flash.h>
}

#include "../persistence.h"

#define PERSISTENT_SIZE         ((512 + 3) & (~3))
#define PERSISTENT_HEADER_LEN   4

#ifndef SPI_FLASH_SEC_SIZE
#define SPI_FLASH_SEC_SIZE      4096
#endif

extern "C" uint32_t _SPIFFS_end;

SCONST char _header[] PROGMEM = "Home";

struct persist_info{
    const uint32_t sector = ((uint32_t)(&_SPIFFS_end - 0x40200000)) / SPI_FLASH_SEC_SIZE;

    persist_info() {
        char cmp_header[PERSISTENT_HEADER_LEN];

        noInterrupts();
        spi_flash_read(sector * SPI_FLASH_SEC_SIZE, reinterpret_cast<uint32_t*>(cmp_header), PERSISTENT_HEADER_LEN);
        interrupts();

        if(memcmp_P(cmp_header, _header, PERSISTENT_HEADER_LEN) != 0){
            hap_persistence_format(this);
        }
    }

    unsigned int offset(unsigned int addr) const {
        return sector * SPI_FLASH_SEC_SIZE + PERSISTENT_HEADER_LEN + addr;
    }
};

void * hap_persistence_init(){
    static persist_info info = persist_info();
    return &info;
}

bool hap_persistence_read(void * _, unsigned int address, uint8_t *buffer, unsigned int length){
    auto info = static_cast<persist_info*>(_);
    noInterrupts();
    spi_flash_read(info->offset(address), reinterpret_cast<uint32_t*>(buffer), length);
    interrupts();
    return true;
}

bool hap_persistence_write(void * _, unsigned int address, const uint8_t *buffer, unsigned int length){
    auto info = static_cast<persist_info*>(_);
    noInterrupts();
    spi_flash_write(info->offset(address), const_cast<uint32_t*>(reinterpret_cast<const uint32_t*>(buffer)), length);
    interrupts();
    return true;
}

void hap_persistence_deinit(void *){ }

void hap_persistence_format(void * _){
    auto info = static_cast<persist_info*>(_);
    char hdr[PERSISTENT_HEADER_LEN];
    char zeros[16]; //Only write 16 bytes of zeros to cover the flags
    memcpy_P(hdr, _header, PERSISTENT_HEADER_LEN);
    memset(zeros, 0, sizeof(zeros));

    noInterrupts();
    spi_flash_write(info->offset(0), reinterpret_cast<uint32_t*>(hdr), PERSISTENT_HEADER_LEN);
    spi_flash_write(info->offset(PERSISTENT_HEADER_LEN), reinterpret_cast<uint32_t*>(zeros), sizeof(zeros));
    interrupts();
}

#endif
