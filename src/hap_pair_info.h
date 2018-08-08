//
// Created by Xule Zhou on 8/6/18.
//

#ifndef ARDUINOHOMEKIT_HAP_PAIR_INFO_H
#define ARDUINOHOMEKIT_HAP_PAIR_INFO_H

struct hap_pair_info{
public:
    explicit hap_pair_info(HAPServer *);
    ~hap_pair_info();
    bool paired();
    hap_crypto_info * prepare(bool isWrite, hap_network_connection *);

private:
    friend class HAPServer;
    friend class HAPPairingsManager;
    friend class HAPUserHelper;

    bool isPaired = false;
    bool isVerifying = false;
    bool isPairing = false;
    uint8_t currentStep = 0;

    //Only used during setup/verify
    hap_crypto_setup * setupStore = nullptr;
    hap_crypto_info * infoStore = nullptr;
    hap_crypto_verify * verifyStore = nullptr;

    uint8_t identifier[36];
    uint8_t AccessoryToControllerKey[32];
    uint8_t ControllerToAccessoryKey[32];
    uint8_t nonceStore[8];
    uint32_t writeCount = 0;
    uint32_t readCount = 0;

    HAPServer * server;

    void renewInfoStore(HAPUserHelper *);
};

#endif //ARDUINOHOMEKIT_HAP_PAIR_INFO_H
