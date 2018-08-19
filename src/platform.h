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
