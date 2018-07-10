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
#define NATIVE_STRINGS

//for compatibility
#define PROGMEM
#define PSTR(s) s
#define F(s) s
#define strlen_P strlen
#define strncasecmp_P strncasecmp
#define memcpy_P memcpy
#define sprintf_P sprintf
#define snprintf_P snprintf
#endif

#endif