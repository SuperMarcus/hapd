#ifdef __cplusplus
extern "C" {
#endif
/*
 * Secure Remote Password 6a implementation based on mbedtls.
 *
 * Copyright (c) 2015 Dieter Wimberger
 * https://github.com/dwimberger/mbedtls-csrp
 * 
 * Derived from:
 * Copyright (c) 2010 Tom Cocagne. All rights reserved.
 * https://github.com/cocagne/csrp
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Tom Cocagne, Dieter Wimberger
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/* 
 * 
 * Purpose:       This is a direct implementation of the Secure Remote Password
 *                Protocol version 6a as described by 
 *                http://srp.stanford.edu/design.html
 * 
 * Author:        tom.cocagne@gmail.com (Tom Cocagne), Dieter Wimberger
 * 
 * Dependencies:  mbedtls
 * 
 * Usage:         Refer to test_srp.c for a demonstration
 * 
 * Notes:
 *    This library allows multiple combinations of hashing algorithms and 
 *    prime number constants. For authentication to succeed, the hash and
 *    prime number constants must match between 
 *    srp_create_salted_verification_key(), srp_user_new(),
 *    and srp_verifier_new(). A recommended approach is to determine the
 *    desired level of security for an application and globally define the
 *    hash and prime number constants to the predetermined values.
 * 
 *    As one might suspect, more bits means more security. As one might also
 *    suspect, more bits also means more processing time. The test_srp.c 
 *    program can be easily modified to profile various combinations of 
 *    hash & prime number pairings.
 */

#ifndef SRP_H
#define SRP_H

//#define SHA1_DIGEST_LENGTH 20
//#define SHA224_DIGEST_LENGTH 224
//#define SHA256_DIGEST_LENGTH 256
//#define SHA384_DIGEST_LENGTH 384
#define SHA512_DIGEST_LENGTH 64
#define BIGNUM	mbedtls_mpi

typedef enum 
{
    SRP_SHA512
} SRP_HashAlgorithm;

/**
 * Manually exported functions for HAP
 */

typedef struct {
	BIGNUM *N;
	BIGNUM *g;
} NGConstant;

struct SRPVerifier {
	SRP_HashAlgorithm hash_alg;
	NGConstant *ng;

	const char *username;
	const unsigned char *bytes_B;
	int authenticated;

	unsigned char M[SHA512_DIGEST_LENGTH];
	unsigned char H_AMK[SHA512_DIGEST_LENGTH];
	unsigned char session_key[SHA512_DIGEST_LENGTH];
};

void delete_ng(NGConstant *ng);
void csrp_init_random();
mbedtls_ctr_drbg_context * csrp_ctr_drbg_ctx();
mbedtls_mpi * csrp_speed_RR();

#endif /* Include Guard */
#ifdef __cplusplus
}
#endif
