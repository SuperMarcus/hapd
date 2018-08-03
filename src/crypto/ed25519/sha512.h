#ifndef SHA512_H
#define SHA512_H

#include <stddef.h>

#include "fixedint.h"
#include "../sha512.h"

/* state */
typedef mbedtls_sha512_context sha512_context;

int sha512_init(sha512_context * md);
int sha512_final(sha512_context * md, unsigned char *out);
int sha512_update(sha512_context * md, const unsigned char *in, size_t inlen);
int sha512(const unsigned char *message, size_t message_len, unsigned char *out);

#endif
