#ifndef HAVE_PLATFORM_H
#define HAVE_PLATFORM_H

//Detects network framework

//Building with Arduino framework
#ifdef ARDUINO

#ifdef ARDUINO_ARCH_ESP8266
#define USE_HAP_LWIP
#define USE_HARDWARE_SERIAL
#define USE_ESPARDUINO_MDNS
#define USE_SPIFLASH_PERSISTENT
#define CONST_DATA
//#define CONST_DATA __attribute__((aligned(4))) __attribute__((section(.irom.text)))
#define DETERMINED_PLATFORM

/**
 * Must use async math with esp8266, cpu set to 160Mhz is also suggested
 */
#define USE_ASYNC_MATH
#endif

#endif

//Unknown platform, assume we are building on a POSIX-compliant platform
#ifndef DETERMINED_PLATFORM

//Use system printf for HAP_DEBUG
#define USE_PRINTF

//Use BSD style socket for network
#define USE_HAP_NATIVE_SOCKET

//Use fs to store persistent information
#define USE_ANSIC_FD_PERSISTENT

//Disable pgmspace
#define NATIVE_STRINGS

#define CONST_DATA

//for pgmspace compatibility
#define PROGMEM
#define PSTR(s) s
#define F(s) s
#define strlen_P strlen
#define strncasecmp_P strncasecmp
#define memcpy_P memcpy
#define sprintf_P sprintf
#define snprintf_P snprintf
#define strcpy_P strcpy

#endif


#endif
