#ifndef HAVE_PLATFORM_H
#define HAVE_PLATFORM_H

//Detects network framework

//Building with Arduino framework
#ifdef ARDUINO

#ifdef ARDUINO_ARCH_ESP8266
#define USE_HAP_LWIP
#define USE_HARDWARE_SERIAL
#define DETERMINED_PLATFORM
#endif

#endif

//Unknown platform
#ifndef DETERMINED_PLATFORM
#define USE_PRINTF
#define USE_HAP_NATIVE_SOCKET

#define PROGMEM
#define strlen_P strlen
#define strncasecmp_P strncasecmp
#endif

#endif
