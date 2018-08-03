#include "sha512.h"

int sha512_init(sha512_context * md){
    mbedtls_sha512_init(md);
    return mbedtls_sha512_starts_ret(md, 0);
}

int sha512_final(sha512_context * md, unsigned char *out){
    return mbedtls_sha512_finish_ret(md, out);
}

int sha512_update(sha512_context * md, const unsigned char *in, size_t inlen){
    return mbedtls_sha512_update_ret(md, in, inlen);
}

int sha512(const unsigned char *message, size_t message_len, unsigned char *out){
    return mbedtls_sha512_ret(message, (size_t) message, out, 0);
}
