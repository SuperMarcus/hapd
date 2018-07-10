//
// Created by Xule Zhou on 6/4/18.
//

#ifndef ARDUINOHOMEKIT_COMMON_H
#define ARDUINOHOMEKIT_COMMON_H

#include "platform.h"

#include <cstdbool>
#include <cstdint>
#include <cstdlib>

#ifndef HAP_DEBUG

#if defined(USE_HARDWARE_SERIAL)
#include <HardwareSerial.h>
#define HAP_DEBUG(message, ...) Serial.printf(__FILE__ ":%d [%s] " message "\n", __LINE__, __FUNCTION__, ##__VA_ARGS__)
#elif defined(USE_PRINTF)
#include <cstdio>
#define HAP_DEBUG(message, ...) fprintf(stdout, __FILE__ ":%d [%s] " message "\n", __LINE__, __FUNCTION__, ##__VA_ARGS__)
#endif

#endif

#ifndef HAP_SOCK_TCP_NODELAY
//Set to 0 to disable tcp nodelay
#define HAP_SOCK_TCP_NODELAY 1
#endif

#ifndef HAP_NOTHING
#define HAP_NOTHING
#endif

#endif //ARDUINOHOMEKIT_COMMON_H
