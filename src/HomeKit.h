#include <Arduino.h>

#include "network.h"

class HomeKit {
private:
    HomeKit();//NOLINT

public:
    static HomeKit shared;
};
