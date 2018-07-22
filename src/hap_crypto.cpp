//
// Created by Xule Zhou on 7/17/18.
//

#include "hap_crypto.h"
#include "crypto/ctr_drbg.h"
#include "crypto/bignum.h"
#include "crypto/srp.h"
#include "HomeKitAccessory.h"

#include <cstring>

static const char _N[] PROGMEM =
        "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E08"
        "8A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B"
        "302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9"
        "A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE6"
        "49286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8"
        "FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D"
        "670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C"
        "180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF695581718"
        "3995497CEA956AE515D2261898FA051015728E5A8AAAC42DAD33170D"
        "04507A33A85521ABDF1CBA64ECFB850458DBEF0A8AEA71575D060C7D"
        "B3970F85A6E1E4C7ABF5AE8CDB0933D71E8C94E04A25619DCEE3D226"
        "1AD2EE6BF12FFA06D98A0864D87602733EC86A64521F2B18177B200C"
        "BBE117577A615D6C770988C0BAD946E208E24FA074E5AB3143DB5BFC"
        "E0FD108E4B82D120A93AD2CAFFFFFFFFFFFFFFFF";

static const char _g[] PROGMEM =
        "5";

NGConstant * _hap_read_ng(){
    auto NLen = strlen_P(_N), gLen = strlen_P(_g);
    auto N = new char[NLen + 1](), g = new char[gLen + 1]();
    memcpy_P(N, _N, NLen);
    memcpy_P(g, _g, gLen);

    auto ng = new_ng(SRP_NG_CUSTOM, N, g);
    delete[] N;
    delete[] g;

    return ng;
}

/**
 * Generate salt and verifier, then emits HAPCRYPTO_SRP_INIT_FINISH_GEN_SALT
 *
 * @param info
 */
void hap_crypto_srp_init(hap_crypto_setup * info) {
    auto ng = _hap_read_ng();
    auto pass = info->server->setupCode;
    int saltLen;

    srp_create_salted_verification_key(
            SRP_SHA512, "Pair-Setup",
            reinterpret_cast<const unsigned char *>(pass), static_cast<int>(strlen(pass)),
            &info->salt, &saltLen, &info->verifier, reinterpret_cast<int *>(&info->verifierLen),
            ng);
    if(saltLen != 16){
        HAP_DEBUG("WARN: salt len is not 16");
    }
    delete_ng(ng);
    info->server->emit(HAPEvent::HAPCRYPTO_SRP_INIT_FINISH_GEN_SALT, info);
}

/**
 * Generate public key and emits HAPCRYPTO_SRP_INIT_COMPLETE
 *
 * @param event
 */
void _srpInit_onGenSalt_thenGenPub(HAPEvent * event){
    auto info = event->arg<hap_crypto_setup>();
    auto ng = _hap_read_ng();
    csrp_init_random();

    //Generate private key, b
    auto b = new mbedtls_mpi;
    mbedtls_mpi_init(b);
    mbedtls_mpi_fill_random(b, 32, &mbedtls_ctr_drbg_random, csrp_ctr_drbg_ctx());

    //Calculate k
    auto k = H_nn(SRP_SHA512, ng->N, ng->g);
    auto v = new mbedtls_mpi;
    auto tmp1 = new mbedtls_mpi;
    auto tmp2 = new mbedtls_mpi;

    mbedtls_mpi_init(v);
    mbedtls_mpi_init(tmp1);
    mbedtls_mpi_init(tmp2);
    mbedtls_mpi_read_binary(v, info->verifier, info->verifierLen);

    //Calculate two fragments of B
    mbedtls_mpi_mul_mpi(tmp1, k, v);
    mbedtls_mpi_exp_mod(tmp2, ng->g, b, ng->N, csrp_speed_RR());

    //Free ng, k, and v
    mbedtls_mpi_free(k);
    mbedtls_mpi_free(v);
    delete v;
    delete k;

    //Export b
    auto bLen = mbedtls_mpi_size(b);
    auto bBytes = new uint8_t[bLen];
    mbedtls_mpi_write_binary(b, bBytes, bLen);
    info->b = bBytes;
    info->bLen = static_cast<uint16_t>(bLen);

    //Free b
    mbedtls_mpi_free(b);
    delete b;

    //Calculate B, the public key
    auto B = new mbedtls_mpi;
    mbedtls_mpi_init(B);
    mbedtls_mpi_add_mpi(B, tmp1, tmp2);
    mbedtls_mpi_mod_mpi(B, B, ng->N);
    delete_ng(ng);

    //Free tmp mpis
    mbedtls_mpi_free(tmp1);
    mbedtls_mpi_free(tmp2);
    delete tmp1;
    delete tmp2;

    //Export B
    auto BLen = mbedtls_mpi_size(B);
    auto BBytes = new uint8_t[BLen];
    mbedtls_mpi_write_binary(B, BBytes, BLen);
    info->B = BBytes;
    info->BLen = static_cast<uint16_t>(BLen);

    //Free B
    mbedtls_mpi_free(B);
    delete B;

    info->server->emit(HAPEvent::HAPCRYPTO_SRP_INIT_COMPLETE, info);
}

void hap_crypto_init(HAPServer * server) {
    server->on(HAPEvent::HAPCRYPTO_SRP_INIT_FINISH_GEN_SALT, _srpInit_onGenSalt_thenGenPub);
}

hap_crypto_setup::hap_crypto_setup(HAPServer *server): server(server) {

}

hap_crypto_setup::~hap_crypto_setup() {
    auto s = const_cast<uint8_t *>(salt);
    delete[] s;

    auto v = const_cast<uint8_t *>(verifier);
    delete[] v;

    auto _b = const_cast<uint8_t *>(b);
    delete[] _b;

    auto _B = const_cast<uint8_t *>(B);
    delete[] _B;
}
