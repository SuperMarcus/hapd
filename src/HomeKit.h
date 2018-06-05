#include <Arduino.h>

#include "network.h"
#include "tlv.h"

class HomeKit {
private:
    HomeKit();//NOLINT

public:
    static HomeKit shared;
};
