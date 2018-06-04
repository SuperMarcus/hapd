//
// Created by Xule Zhou on 6/4/18.
//

#ifndef ARDUINOHOMEKIT_COMMON_H
#define ARDUINOHOMEKIT_COMMON_H

#include <cstdbool>
#include <cstdint>
#include <cstdlib>

#ifndef HAP_DEBUG
#include <HardwareSerial.h>
#define HAP_DEBUG Serial.printf
#endif

#ifndef HAP_LWIP_TCP_NODELAY
//Set to 0 to disable tcp nodelay
#define HAP_LWIP_TCP_NODELAY 1
#endif

#endif //ARDUINOHOMEKIT_COMMON_H
