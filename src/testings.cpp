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

#else

#include <Arduino.h>
#include <ESP8266WiFi.h>

void setup(){
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    WiFi.begin("Xiaomi_CJ", "86028560");
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
