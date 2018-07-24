//
// Created by Xule Zhou on 7/17/18.
//

#ifndef ARDUINOHOMEKIT_HAP_CRYPTO_H
#define ARDUINOHOMEKIT_HAP_CRYPTO_H

#include "common.h"

#define HAPCRYPTO_SRP_MODULUS_SIZE      384
#define HAPCRYPTO_SRP_GENERATOR_SIZE    1
#define HAPCRYPTO_SALT_SIZE             16
#define HAPCRYPTO_SHA_SIZE              64

class HAPUserHelper;

struct hap_crypto_setup {
    const uint8_t * salt = nullptr;
    const uint8_t * verifier = nullptr;
    const uint8_t * b = nullptr;
    const uint8_t * B = nullptr;
    const uint8_t * A = nullptr;
    const uint8_t * sessionKey = nullptr;
    const uint8_t * clientProof = nullptr;
    const uint8_t * serverProof = nullptr;

    unsigned int verifierLen = 0;
    uint16_t bLen = 0;
    uint16_t BLen = 0;
    uint16_t ALen = 0;

    void * handle = nullptr;
    HAPUserHelper * session = nullptr;

    //Only the fields below are initialized by HAPServer
    HAPServer * server;
    const char * username;
    const char * password;

    explicit hap_crypto_setup(HAPServer * server, const char * user, const char * pass);
    ~hap_crypto_setup();
};

/**
 * Add crypto yield listeners to server. Called by
 * HAPPairingsManager::HAPPairingsManager()
 */
void hap_crypto_init(HAPServer *);

/**
 * Init srp, generate the 16bytes salt and store it
 * in hap_crypto_setup.salt.
 *
 * Upon completion, emit HAPCRYPTO_SRP_INIT_COMPLETE
 * in the provided HAPServer
 */
void hap_crypto_srp_init(hap_crypto_setup *);

/**
 * Verify client's srp proof and public key
 */
void hap_crypto_srp_proof(hap_crypto_setup *);

/**
 * Verify the client's proof with calculated M
 *
 * @return true if authenticated
 */
bool hap_crypto_verify_client_proof(hap_crypto_setup *);

/**
 * Free the handle inside setup info
 */
void hap_crypto_srp_free(hap_crypto_setup *);

#endif //ARDUINOHOMEKIT_HAP_CRYPTO_H
