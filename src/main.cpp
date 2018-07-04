#include "HAPServer.h"

#define HAP_TESTING
#ifdef HAP_TESTING

void testLwipNetwork(){
    Serial.printf("[test] testing lwip hap network implementation...\n");

    hap_network_connection conn {};
    hap_network_init_bind(&conn, 80);
}

void _setup();

void setup(){
    delay(2000);

    Serial.begin(115200);
    Serial.setDebugOutput(true);
    _setup();

    Serial.printf("[test] Testing...\n");

//    testLwipNetwork();

    Serial.printf("[test] All tests have been completed.\n");
}

void loop(){
    delay(500);
}

#include <ESP8266WiFi.h>

void _setup(){
    Serial.printf("[boot] Setting up...\n");

    WiFi.hostname("esp-testing-device");
    WiFi.begin();

    while(WiFi.status() != WL_CONNECTED){
        Serial.printf("[wifi] Waiting for network connection...\n");
        delay(2000);
    }
}

#endif
