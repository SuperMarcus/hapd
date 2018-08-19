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

#include "../common.h"

#ifdef USE_ANSIC_FD_PERSISTENT

#include <cstdio>
#include <cstring>
#include <cerrno>
#include "../persistence.h"

#define HAP_ANSIC_ERRLOG(f) HAP_DEBUG(f "(%d): %s", errno, strerror(errno))

struct hap_posix_fd {
    FILE * fd;
    unsigned int startOffset;
};

static const char filename[] = ".accessory";
static const char header[] = "HAP-PersistingStorageData:";

void * hap_persistence_init(){
    auto handle = new hap_posix_fd();
    handle->startOffset = sizeof(header) - 1;

    handle->fd = fopen(filename, "rb+");
    if(!handle->fd) handle->fd = fopen(filename, "wb+");

    if(!handle->fd){
        HAP_ANSIC_ERRLOG("fopen");
        delete handle;
        return nullptr;
    }

    char buf[sizeof(header)];
    memset(buf, 0, sizeof(header));
    fread(buf, handle->startOffset, 1, handle->fd);

    //Not initialized, allocate fixed section
    if(strcmp(header, buf) != 0){ hap_persistence_format(handle); }

    return handle;
}

bool hap_persistence_read(void * _, unsigned int address, uint8_t *buffer, unsigned int length){
    auto handle = static_cast<hap_posix_fd*>(_);

    auto ret = fseek(handle->fd, handle->startOffset + address, SEEK_SET);
    if(ret != 0){
        HAP_ANSIC_ERRLOG("fseek");
        return false;
    }

    auto readBytes = fread(buffer, sizeof(uint8_t), length, handle->fd);
    if(readBytes != length){
        HAP_ANSIC_ERRLOG("fread");
        return false;
    }

    return true;
}

bool hap_persistence_write(void * _, unsigned int address, const uint8_t *buffer, unsigned int length){
    auto handle = static_cast<hap_posix_fd*>(_);

    auto ret = fseek(handle->fd, handle->startOffset + address, SEEK_SET);
    if(ret != 0){
        HAP_ANSIC_ERRLOG("fseek");
        return false;
    }

    auto writeBytes = fwrite(buffer, sizeof(uint8_t), length, handle->fd);
    if(writeBytes != length){
        HAP_ANSIC_ERRLOG("fwrite");
        return false;
    }

    fflush(handle->fd);

    return true;
}

void hap_persistence_deinit(void * _){
    auto handle = static_cast<hap_posix_fd*>(_);
    fclose(handle->fd);
    delete handle;
}

void hap_persistence_format(void * _) {
    auto handle = static_cast<hap_posix_fd*>(_);
    fclose(handle->fd);

    //Create new empty file
    handle->fd = fopen(filename, "wb+");
    if(!handle->fd){
        HAP_ANSIC_ERRLOG("fopen");
        return;
    }

    auto writeBytes = fwrite(header, sizeof(char), handle->startOffset, handle->fd);
    if(writeBytes != handle->startOffset){
        HAP_ANSIC_ERRLOG("fwrite");
        return;
    }

    auto zeros = new char[HAP_FIXED_BLOCK_SIZE];
    memset(zeros, 0, HAP_FIXED_BLOCK_SIZE);

    writeBytes = fwrite(zeros, sizeof(char), HAP_FIXED_BLOCK_SIZE, handle->fd);
    if(writeBytes != HAP_FIXED_BLOCK_SIZE){
        HAP_ANSIC_ERRLOG("fwrite");
        return;
    }

    delete[] zeros;
}

#endif
