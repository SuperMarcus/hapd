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

#include "HomeKitAccessory.h"
#include "service/Switch.h"

#ifdef HAP_NATIVE_DEBUG_ENV

#include <pthread/pthread.h>
#include <unistd.h>
#include <cstdio>

int main(){
    HAP_DEBUG("Testing HAP with native socket...");

    HKAccessory.begin();
    HKAccessory.getAccessory()->addService<Switch>();

    while (1){
        HKAccessory.handle();
        usleep(2e4);
    }
}

#endif

#ifdef HAP_ESP8266_DEBUG_ENV

#include <Arduino.h>
#include <ESP8266WiFi.h>

void setup(){
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    WiFi.begin();
    HAP_DEBUG("wlan connecting");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(200);
    }
    Serial.println();

    HAP_DEBUG("setup");
    HKAccessory.begin();
    HAP_DEBUG("end");
}

void loop(){
    HKAccessory.handle();
}

#endif
