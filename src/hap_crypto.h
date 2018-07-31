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
#define HAPCRYPTO_CHACHA_KEYSIZE        32

class HAPUserHelper;

struct hap_crypto_info {
    HAPServer * server;
    HAPUserHelper * session;

    uint8_t * encryptedData = nullptr;
    uint8_t * authTag = nullptr;
    uint8_t * rawData = nullptr;

    const uint8_t * nonce = nullptr;
    const uint8_t * aad = nullptr;

    uint8_t key[HAPCRYPTO_CHACHA_KEYSIZE];

    unsigned int dataLen = 0;
    unsigned int nonceLen = 0;
    unsigned int aadLen = 0;

    void reset();

    hap_crypto_info(HAPServer *, HAPUserHelper *);
    ~hap_crypto_info();
};

struct hap_crypto_setup {
    const uint8_t * salt = nullptr;
    const uint8_t * verifier = nullptr;
    const uint8_t * b = nullptr;
    const uint8_t * B = nullptr;
    const uint8_t * A = nullptr;
    const uint8_t * sessionKey = nullptr;
    const uint8_t * clientProof = nullptr;
    const uint8_t * serverProof = nullptr;
    const uint8_t * deviceLtpk = nullptr;

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
 * Async function
 *
 * Init srp, generate the 16bytes salt and store it
 * in hap_crypto_setup.salt.
 *
 * Upon completion, emit HAPCRYPTO_SRP_INIT_COMPLETE
 * in the provided HAPServer
 */
void hap_crypto_srp_init(hap_crypto_setup *);

/**
 * Async function
 *
 * Verify client's srp proof and public key
 */
void hap_crypto_srp_proof(hap_crypto_setup *);

/**
 * Synchronized function
 *
 * Verify the client's proof with calculated M
 *
 * @return true if authenticated
 */
bool hap_crypto_verify_client_proof(hap_crypto_setup *);

/**
 * Synchronized function
 *
 * Free the handle inside setup info
 */
void hap_crypto_srp_free(hap_crypto_setup *);

/**
 * Async function
 *
 * Chacha20-Poly1305 decrypt and verify
 *
 * Decrypt data in encryptedData buffer, allocate rawData buffer
 * and free the encryptedData. Nonce is automatically left padded
 * with \x00. When finishes, emits HAPCRYPTO_DECRYPTED
 *
 * If decryption fails or verification fails, encryptedData is not
 * freed. Use hap_crypto_data_decrypt_did_succeed() to check if
 * data is authenticated.
 */
void hap_crypto_data_decrypt(hap_crypto_info *);

/**
 * Async function
 *
 * Chacha20-Poly1305 encrypt and tag
 *
 * Encrypt raw data, allocate encryptedData buffer and authTag,
 * and free the rawData buffer. When finishes, emit
 * HAPCRYPTO_ENCRYPTED
 */
void hap_crypto_data_encrypt(hap_crypto_info *);

/**
 * Synchronized function
 *
 * @return true if decryption succeeds
 */
bool hap_crypto_data_decrypt_did_succeed(hap_crypto_info *);

/**
 * Synchronized function, maybe change it to async later?
 *
 * Derive the 32 bytes key with HKDF-SHA-512, and
 *
 * @param dst 32 bytes destination buffer, must be allocated
 * @param input Input key of size #HAPCRYPTO_SHA_SIZE
 * @param salt The cstring salt, end with \x00
 * @param info  Cstring info, end with \x00
 */
void hap_crypto_derive_key(uint8_t * dst, const uint8_t * input, const char * salt, const char * info);

/**
 * Verify ed25519 signature
 *
 * @param signature 64 bytes signature
 * @param message Message to be verified
 * @param len Length of message
 * @param pubKey Public key
 * @return
 */
bool hap_crypto_verify(uint8_t * signature, uint8_t * message, unsigned int len, uint8_t * pubKey);

/**
 * Sign the message with ed25519
 *
 * @param message Message to be signed
 * @param len Length of message
 * @param secKey ed25519 secret key
 * @return
 */
uint8_t * hap_crypto_sign(uint8_t * message, unsigned int len, uint8_t * pubKey, uint8_t * secKey);

/**
 * Synchronized function, since its mild speed
 *
 * Generates ed25519 keypair and store it to the given buffer
 */
void hap_crypto_generate_keypair(uint8_t * publicKey, uint8_t * privateKey);

/**
 * Synchronized function
 *
 * Derive uuid 4 with the first 16 bytes of the sha512 hash
 * of input data.
 *
 * @return The allocated and formatted uuid4 char array
 */
char * hap_crypto_derive_uuid(const char *);

#endif //ARDUINOHOMEKIT_HAP_CRYPTO_H
