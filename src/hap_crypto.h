//
// Created by Xule Zhou on 7/17/18.
//

#ifndef ARDUINOHOMEKIT_HAP_CRYPTO_H
#define ARDUINOHOMEKIT_HAP_CRYPTO_H

#include "common.h"

class HAPUserHelper;

struct hap_crypto_setup {
    const uint8_t * salt = nullptr;
    const uint8_t * verifier = nullptr;
    const uint8_t * b = nullptr;
    const uint8_t * B = nullptr;

    unsigned int verifierLen = 0;
    uint16_t bLen = 0;
    uint16_t BLen = 0;

    void * handle = nullptr;
    HAPUserHelper * session = nullptr;

    //Only the fields below are initialized by HAPServer
    HAPServer * server;

    explicit hap_crypto_setup(HAPServer * server);
    ~hap_crypto_setup();
};

/**
 * Add crypto yield listeners to server. Called by
 * HAPPairingsManager::HAPPairingsManager()
 */
void hap_crypto_init(HAPServer *);

/**
 * Init srp, generate the 16bytes salt and store it
 * in hap_crypto_setup.salt
 */
void hap_crypto_srp_init(hap_crypto_setup *);

#endif //ARDUINOHOMEKIT_HAP_CRYPTO_H
