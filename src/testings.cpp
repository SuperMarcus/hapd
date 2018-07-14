#include "HomeKitAccessory.h"

#ifdef HAP_NATIVE_DEBUG_ENV

#include <pthread/pthread.h>
#include <unistd.h>

int main(){
    HAP_DEBUG("Testing HAP with native socket...");

    HKAccessory.begin();

    while (1){
        HKAccessory.handle();
        usleep(2e4);
    }
}

#else

#include <Arduino.h>

void setup(){

}

void loop(){

}

#endif
