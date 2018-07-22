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

#include <stdlib.h>
#include <string.h>

#include "bignum.h"
#include "entropy.h"
#include "ctr_drbg.h"
#include "sha512.h"

#include "srp.h"

static int g_initialized = 0;
static mbedtls_entropy_context entropy_ctx;
static mbedtls_ctr_drbg_context ctr_drbg_ctx;
static mbedtls_mpi *RR;

/* All constants here were pulled from Appendix A of RFC 5054 */
static struct NGHex global_Ng_constants[] = {
        {       0, 0} /* null sentinel */
};

NGConstant *new_ng(SRP_NGType ng_type, const char *n_hex, const char *g_hex) {
    NGConstant *ng = (NGConstant *) malloc(sizeof(NGConstant));
    ng->N = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    ng->g = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(ng->N);
    mbedtls_mpi_init(ng->g);

    if (!ng || !ng->N || !ng->g)
        return 0;

    mbedtls_mpi_read_string(ng->N, 16, n_hex);
    mbedtls_mpi_read_string(ng->g, 16, g_hex);

    return ng;
}

void delete_ng(NGConstant *ng) {
    if (ng) {
        mbedtls_mpi_free(ng->N);
        mbedtls_mpi_free(ng->g);
        free(ng->N);
        free(ng->g);
        free(ng);
    }
}


typedef union {
    mbedtls_sha512_context sha512;
} HashCTX;


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


struct SRPUser {
    SRP_HashAlgorithm hash_alg;
    NGConstant *ng;

    BIGNUM *a;
    BIGNUM *A;
    BIGNUM *S;

    const unsigned char *bytes_A;
    int authenticated;

    const char *username;
    const unsigned char *password;
    int password_len;

    unsigned char M[SHA512_DIGEST_LENGTH];
    unsigned char H_AMK[SHA512_DIGEST_LENGTH];
    unsigned char session_key[SHA512_DIGEST_LENGTH];
};


static void hash_init(SRP_HashAlgorithm alg, HashCTX *c) {
    switch (alg) {
//        case SRP_SHA1  :
//            mbedtls_sha1_init(&c->sha);
//        case SRP_SHA256:
//            mbedtls_sha256_init(&c->sha256);
        case SRP_SHA512:
            mbedtls_sha512_init(&c->sha512);
        default:
            return;
    };
}

static void hash_start(SRP_HashAlgorithm alg, HashCTX *c) {
    switch (alg) {
//        case SRP_SHA1  :
//            mbedtls_sha1_starts(&c->sha);
//        case SRP_SHA224:
//            mbedtls_sha256_starts(&c->sha256, 1);
//        case SRP_SHA256:
//            mbedtls_sha256_starts(&c->sha256, 0);
//        case SRP_SHA384:
//            mbedtls_sha512_starts(&c->sha512, 1);
        case SRP_SHA512:
            mbedtls_sha512_starts(&c->sha512, 0);
        default:
            return;
    };
}


static void hash_update(SRP_HashAlgorithm alg, HashCTX *c, const void *data, size_t len) {
    switch (alg) {
//        case SRP_SHA1  :
//            mbedtls_sha1_update(&c->sha, data, len);
//            break;
//        case SRP_SHA224:
//            mbedtls_sha256_update(&c->sha256, data, len);
//            break;
//        case SRP_SHA256:
//            mbedtls_sha256_update(&c->sha256, data, len);
//            break;
//        case SRP_SHA384:
//            mbedtls_sha512_update(&c->sha512, data, len);
//            break;
        case SRP_SHA512:
            mbedtls_sha512_update(&c->sha512, data, len);
            break;
        default:
            return;
    };
}

static void hash_final(SRP_HashAlgorithm alg, HashCTX *c, unsigned char *md) {
    switch (alg) {
//        case SRP_SHA1  :
//            mbedtls_sha1_finish(&c->sha, md);
//            break;
//        case SRP_SHA224:
//            mbedtls_sha256_finish(&c->sha256, md);
//            break;
//        case SRP_SHA256:
//            mbedtls_sha256_finish(&c->sha256, md);
//            break;
//        case SRP_SHA384:
//            mbedtls_sha512_finish(&c->sha512, md);
//            break;
        case SRP_SHA512:
            mbedtls_sha512_finish(&c->sha512, md);
            break;
        default:
            return;
    };
}

static void hash(SRP_HashAlgorithm alg, const unsigned char *d, size_t n, unsigned char *md) {
    switch (alg) {
//        case SRP_SHA1  :
//            mbedtls_sha1(d, n, md);
//            break;
//        case SRP_SHA224:
//            mbedtls_sha256(d, n, md, 1);
//            break;
//        case SRP_SHA256:
//            mbedtls_sha256(d, n, md, 0);
//            break;
//        case SRP_SHA384:
//            mbedtls_sha512(d, n, md, 1);
//            break;
        case SRP_SHA512:
            mbedtls_sha512(d, n, md, 0);
            break;
        default:
            return;
    };
}

static int hash_length(SRP_HashAlgorithm alg) {
    switch (alg) {
//        case SRP_SHA1  :
//            return SHA1_DIGEST_LENGTH;
//        case SRP_SHA224:
//            return SHA224_DIGEST_LENGTH;
//        case SRP_SHA256:
//            return SHA256_DIGEST_LENGTH;
//        case SRP_SHA384:
//            return SHA384_DIGEST_LENGTH;
        case SRP_SHA512:
            return SHA512_DIGEST_LENGTH;
        default:
            return -1;
    };
}


BIGNUM *H_nn(SRP_HashAlgorithm alg, const BIGNUM *n1, const BIGNUM *n2) {
    unsigned char buff[SHA512_DIGEST_LENGTH];
    int len_n1 = mbedtls_mpi_size(n1);
    int len_n2 = mbedtls_mpi_size(n2);
    int nbytes = len_n1 + len_n2;
    unsigned char *bin = (unsigned char *) malloc(nbytes);
    if (!bin)
        return 0;

    mbedtls_mpi_write_binary(n1, bin, len_n1);
    mbedtls_mpi_write_binary(n2, bin + len_n1, len_n2);
    hash(alg, bin, nbytes, buff);
    free(bin);
    BIGNUM *bn;
    bn = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(bn);
    mbedtls_mpi_read_binary(bn, buff, hash_length(alg));
    return bn;
}

BIGNUM *H_ns(SRP_HashAlgorithm alg, const BIGNUM *n, const unsigned char *bytes, int len_bytes) {
    unsigned char buff[SHA512_DIGEST_LENGTH];
    int len_n = mbedtls_mpi_size(n);
    int nbytes = len_n + len_bytes;
    unsigned char *bin = (unsigned char *) malloc(nbytes);
    if (!bin)
        return 0;
    mbedtls_mpi_write_binary(n, bin, len_n);
    memcpy(bin + len_n, bytes, len_bytes);
    hash(alg, bin, nbytes, buff);
    free(bin);

    BIGNUM *bn;
    bn = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(bn);
    mbedtls_mpi_read_binary(bn, buff, hash_length(alg));
    return bn;
}

static BIGNUM *
calculate_x(SRP_HashAlgorithm alg, const BIGNUM *salt, const char *username, const unsigned char *password,
            int password_len) {
    unsigned char ucp_hash[SHA512_DIGEST_LENGTH];
    HashCTX ctx;

    hash_init(alg, &ctx);

    hash_update(alg, &ctx, username, strlen(username));
    hash_update(alg, &ctx, ":", 1);
    hash_update(alg, &ctx, password, password_len);

    hash_final(alg, &ctx, ucp_hash);

    return H_ns(alg, salt, ucp_hash, hash_length(alg));
}

static void update_hash_n(SRP_HashAlgorithm alg, HashCTX *ctx, const BIGNUM *n) {
    unsigned long len = mbedtls_mpi_size(n);
    unsigned char *n_bytes = (unsigned char *) malloc(len);
    if (!n_bytes)
        return;
    mbedtls_mpi_write_binary(n, n_bytes, len);
    hash_update(alg, ctx, n_bytes, len);
    free(n_bytes);
}

static void hash_num(SRP_HashAlgorithm alg, const BIGNUM *n, unsigned char *dest) {
    int nbytes = mbedtls_mpi_size(n);
    unsigned char *bin = (unsigned char *) malloc(nbytes);
    if (!bin)
        return;
    mbedtls_mpi_write_binary(n, bin, nbytes);
    hash(alg, bin, nbytes, dest);
    free(bin);
}

static void calculate_M(SRP_HashAlgorithm alg, NGConstant *ng, unsigned char *dest, const char *I, const BIGNUM *s,
                        const BIGNUM *A, const BIGNUM *B, const unsigned char *K) {
    unsigned char H_N[SHA512_DIGEST_LENGTH];
    unsigned char H_g[SHA512_DIGEST_LENGTH];
    unsigned char H_I[SHA512_DIGEST_LENGTH];
    unsigned char H_xor[SHA512_DIGEST_LENGTH];
    HashCTX ctx;
    int i = 0;
    int hash_len = hash_length(alg);

    hash_num(alg, ng->N, H_N);
    hash_num(alg, ng->g, H_g);

    hash(alg, (const unsigned char *) I, strlen(I), H_I);


    for (i = 0; i < hash_len; i++)
        H_xor[i] = H_N[i] ^ H_g[i];

    hash_init(alg, &ctx);

    hash_update(alg, &ctx, H_xor, hash_len);
    hash_update(alg, &ctx, H_I, hash_len);
    update_hash_n(alg, &ctx, s);
    update_hash_n(alg, &ctx, A);
    update_hash_n(alg, &ctx, B);
    hash_update(alg, &ctx, K, hash_len);

    hash_final(alg, &ctx, dest);
}

static void calculate_H_AMK(SRP_HashAlgorithm alg, unsigned char *dest, const BIGNUM *A, const unsigned char *M,
                            const unsigned char *K) {
    HashCTX ctx;

    hash_init(alg, &ctx);

    update_hash_n(alg, &ctx, A);
    hash_update(alg, &ctx, M, hash_length(alg));
    hash_update(alg, &ctx, K, hash_length(alg));

    hash_final(alg, &ctx, dest);
}

mbedtls_ctr_drbg_context * csrp_ctr_drbg_ctx(){
    return &ctr_drbg_ctx;
}

mbedtls_mpi * csrp_speed_RR(){
    return RR;
}

#define init_random csrp_init_random
void csrp_init_random() {
    if (g_initialized)
        return;

    mbedtls_entropy_init(&entropy_ctx);
    mbedtls_ctr_drbg_init(&ctr_drbg_ctx);

    unsigned char hotBits[128] = {
            82, 42, 71, 87, 124, 241, 30, 1, 54, 239, 240, 121, 89, 9, 151, 11, 60,
            226, 142, 47, 115, 157, 100, 126, 242, 132, 46, 12, 56, 197, 194, 76,
            198, 122, 90, 241, 255, 43, 120, 209, 69, 21, 195, 212, 100, 251, 18,
            111, 30, 238, 24, 199, 238, 236, 138, 225, 45, 15, 42, 83, 114, 132,
            165, 141, 32, 185, 167, 100, 131, 23, 236, 9, 11, 51, 130, 136, 97, 161,
            36, 174, 129, 234, 2, 54, 119, 184, 70, 103, 118, 109, 122, 15, 24, 23,
            166, 203, 102, 160, 77, 100, 17, 4, 132, 138, 215, 204, 109, 245, 122,
            9, 184, 89, 70, 247, 125, 97, 213, 240, 85, 243, 91, 226, 127, 64, 136,
            37, 154, 232
    };

    mbedtls_ctr_drbg_seed(
            &ctr_drbg_ctx,
            mbedtls_entropy_func,
            &entropy_ctx,
            hotBits,
            128
    );

    RR = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(RR);
    g_initialized = 1;

}


/***********************************************************************************************************
 *
 *  Exported Functions
 *
 ***********************************************************************************************************/

void srp_random_seed(const unsigned char *random_data, int data_length) {
    g_initialized = 1;


    if (mbedtls_ctr_drbg_seed(&ctr_drbg_ctx, mbedtls_entropy_func, &entropy_ctx,
                              (const unsigned char *) random_data,
                              data_length) != 0) {
        return;
    }

}


void srp_create_salted_verification_key(SRP_HashAlgorithm alg,
                                        const char *username,
                                        const unsigned char *password, int len_password,
                                        const unsigned char **bytes_s, int *len_s,
                                        const unsigned char **bytes_v, int *len_v,
                                        NGConstant * ng) {


    BIGNUM *s;
    BIGNUM *v;
    BIGNUM *x = 0;

    s = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(s);
    v = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(v);

    if (!s || !v || !ng)
        goto cleanup_and_exit;

    init_random(); /* Only happens once */

    mbedtls_mpi_fill_random(s, 16,
                            &mbedtls_ctr_drbg_random,
                            &ctr_drbg_ctx);

    x = calculate_x(alg, s, username, password, len_password);

    if (!x)
        goto cleanup_and_exit;

    mbedtls_mpi_exp_mod(v, ng->g, x, ng->N, RR);

    *len_s = mbedtls_mpi_size(s);
    *len_v = mbedtls_mpi_size(v);

    *bytes_s = (const unsigned char *) malloc(*len_s);
    *bytes_v = (const unsigned char *) malloc(*len_v);

    if (!bytes_s || !bytes_v)
        goto cleanup_and_exit;

    mbedtls_mpi_write_binary(s, *bytes_s, *len_s);
    mbedtls_mpi_write_binary(v, *bytes_v, *len_v);

    cleanup_and_exit:
    mbedtls_mpi_free(s);
    free(s);
    mbedtls_mpi_free(v);
    free(v);
    mbedtls_mpi_free(x);
    free(x);
    //TODO: BN_CTX_free(ctx);
}


/* Out: bytes_B, len_B.
 *
 * On failure, bytes_B will be set to NULL and len_B will be set to 0
 */
struct SRPVerifier *srp_verifier_new(SRP_HashAlgorithm alg, SRP_NGType ng_type, const char *username,
                                     const unsigned char *bytes_s, int len_s,
                                     const unsigned char *bytes_v, int len_v,
                                     const unsigned char *bytes_A, int len_A,
                                     const unsigned char **bytes_B, int *len_B,
                                     const char *n_hex, const char *g_hex) {

    BIGNUM *s;
    BIGNUM *v;
    BIGNUM *A;

    s = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(s);
    mbedtls_mpi_read_binary(s, bytes_s, len_s);
    v = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(v);
    mbedtls_mpi_read_binary(v, bytes_v, len_v);
    A = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(A);
    mbedtls_mpi_read_binary(A, bytes_A, len_A);

    BIGNUM *u = 0;
    BIGNUM *B;
    B = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(B);
    BIGNUM *S;
    S = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(S);
    BIGNUM *b;
    b = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(b);
    BIGNUM *k = 0;
    BIGNUM *tmp1;
    tmp1 = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(tmp1);
    BIGNUM *tmp2;
    tmp2 = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(tmp2);
    int ulen = strlen(username) + 1;
    NGConstant *ng = new_ng(ng_type, n_hex, g_hex);
    struct SRPVerifier *ver = 0;

    *len_B = 0;
    *bytes_B = 0;

    if (!s || !v || !A || !B || !S || !b || !tmp1 || !tmp2 || !ng) {
        goto cleanup_and_exit;
    }

    ver = (struct SRPVerifier *) malloc(sizeof(struct SRPVerifier));

    if (!ver) {
        goto cleanup_and_exit;
    }
    init_random(); /* Only happens once */

    ver->username = (char *) malloc(ulen);
    ver->hash_alg = alg;
    ver->ng = ng;

    if (!ver->username) {
        free(ver);
        ver = 0;
        goto cleanup_and_exit;
    }

    memcpy((char *) ver->username, username, ulen);

    ver->authenticated = 0;

    /* SRP-6a safety check */
    mbedtls_mpi_mod_mpi(tmp1, A, ng->N);
    if (mbedtls_mpi_cmp_int(tmp1, 0) != 0) {
        mbedtls_mpi_fill_random(b, 256,
                                &mbedtls_ctr_drbg_random,
                                &ctr_drbg_ctx);

        k = H_nn(alg, ng->N, ng->g);

        /* B = kv + g^b */
        mbedtls_mpi_mul_mpi(tmp1, k, v);
        mbedtls_mpi_exp_mod(tmp2, ng->g, b, ng->N, RR);
        mbedtls_mpi_add_mpi(B, tmp1, tmp2);

        u = H_nn(alg, A, B);

        /* S = (A *(v^u)) ^ b */
        mbedtls_mpi_exp_mod(tmp1, v, u, ng->N, RR);
        mbedtls_mpi_mul_mpi(tmp2, A, tmp1);
        mbedtls_mpi_exp_mod(S, tmp2, b, ng->N, RR);

        hash_num(alg, S, ver->session_key);

        calculate_M(alg, ng, ver->M, username, s, A, B, ver->session_key);
        calculate_H_AMK(alg, ver->H_AMK, A, ver->M, ver->session_key);

        *len_B = mbedtls_mpi_size(B);
        *bytes_B = malloc(*len_B);

        if (!*bytes_B) {
            free((void *) ver->username);
            free(ver);
            ver = 0;
            *len_B = 0;
            goto cleanup_and_exit;
        }

        mbedtls_mpi_write_binary(B, *bytes_B, *len_B);
        ver->bytes_B = *bytes_B;
    }

    cleanup_and_exit:
    mbedtls_mpi_free(s);
    free(s);
    mbedtls_mpi_free(v);
    free(v);
    mbedtls_mpi_free(A);
    free(A);
    if (u) {
        mbedtls_mpi_free(u);
        free(u);
    }
    if (k) {
        mbedtls_mpi_free(k);
        free(k);
    }
    mbedtls_mpi_free(B);
    free(B);
    mbedtls_mpi_free(S);
    free(S);
    mbedtls_mpi_free(b);
    free(b);
    mbedtls_mpi_free(tmp1);
    free(tmp1);
    mbedtls_mpi_free(tmp2);
    free(tmp2);

    return ver;
}


void srp_verifier_delete(struct SRPVerifier *ver) {
    if (ver) {
        delete_ng(ver->ng);
        free((char *) ver->username);
        if (ver->bytes_B != 0) {
            free((unsigned char *) ver->bytes_B);
        }
        memset(ver, 0, sizeof(*ver));
        free(ver);
    }
}


int srp_verifier_is_authenticated(struct SRPVerifier *ver) {
    return ver->authenticated;
}


const char *srp_verifier_get_username(struct SRPVerifier *ver) {
    return ver->username;
}


const unsigned char *srp_verifier_get_session_key(struct SRPVerifier *ver, int *key_length) {
    if (key_length)
        *key_length = hash_length(ver->hash_alg);
    return ver->session_key;
}


int srp_verifier_get_session_key_length(struct SRPVerifier *ver) {
    return hash_length(ver->hash_alg);
}


/* user_M must be exactly SHA512_DIGEST_LENGTH bytes in size */
void
srp_verifier_verify_session(struct SRPVerifier *ver, const unsigned char *user_M, const unsigned char **bytes_HAMK) {
    if (memcmp(ver->M, user_M, hash_length(ver->hash_alg)) == 0) {
        ver->authenticated = 1;
        *bytes_HAMK = ver->H_AMK;
    } else
        *bytes_HAMK = NULL;
}

/*******************************************************************************/

struct SRPUser *srp_user_new(SRP_HashAlgorithm alg, SRP_NGType ng_type, const char *username,
                             const unsigned char *bytes_password, int len_password,
                             const char *n_hex, const char *g_hex) {
    struct SRPUser *usr = (struct SRPUser *) malloc(sizeof(struct SRPUser));
    int ulen = strlen(username) + 1;

    if (!usr)
        goto err_exit;

    init_random(); /* Only happens once */

    usr->hash_alg = alg;
    usr->ng = new_ng(ng_type, n_hex, g_hex);

    usr->a = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    usr->A = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    usr->S = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));

    mbedtls_mpi_init(usr->a);
    mbedtls_mpi_init(usr->A);
    mbedtls_mpi_init(usr->S);

    if (!usr->ng || !usr->a || !usr->A || !usr->S)
        goto err_exit;

    usr->username = (const char *) malloc(ulen);
    usr->password = (const unsigned char *) malloc(len_password);
    usr->password_len = len_password;

    if (!usr->username || !usr->password)
        goto err_exit;

    memcpy((char *) usr->username, username, ulen);
    memcpy((char *) usr->password, bytes_password, len_password);

    usr->authenticated = 0;

    usr->bytes_A = 0;

    return usr;

    err_exit:
    if (usr) {
        mbedtls_mpi_free(usr->a);
        mbedtls_mpi_free(usr->A);
        mbedtls_mpi_free(usr->S);
        if (usr->username)
            free((void *) usr->username);
        if (usr->password) {
            memset((void *) usr->password, 0, usr->password_len);
            free((void *) usr->password);
        }
        free(usr);
    }

    return 0;
}


void srp_user_delete(struct SRPUser *usr) {
    if (usr) {
        mbedtls_mpi_free(usr->a);
        mbedtls_mpi_free(usr->A);
        mbedtls_mpi_free(usr->S);

        delete_ng(usr->ng);

        memset((void *) usr->password, 0, usr->password_len);

        free((char *) usr->username);
        free((char *) usr->password);

        if (usr->bytes_A)
            free((char *) usr->bytes_A);

        memset(usr, 0, sizeof(*usr));
        free(usr);
    }
}


int srp_user_is_authenticated(struct SRPUser *usr) {
    return usr->authenticated;
}


const char *srp_user_get_username(struct SRPUser *usr) {
    return usr->username;
}


const unsigned char *srp_user_get_session_key(struct SRPUser *usr, int *key_length) {
    if (key_length)
        *key_length = hash_length(usr->hash_alg);
    return usr->session_key;
}


int srp_user_get_session_key_length(struct SRPUser *usr) {
    return hash_length(usr->hash_alg);
}


/* Output: username, bytes_A, len_A */
void srp_user_start_authentication(struct SRPUser *usr, const char **username,
                                   const unsigned char **bytes_A, int *len_A) {

    mbedtls_mpi_fill_random(usr->a, 256, &mbedtls_ctr_drbg_random, &ctr_drbg_ctx);
    mbedtls_mpi_exp_mod(usr->A, usr->ng->g, usr->a, usr->ng->N, RR);

    *len_A = mbedtls_mpi_size(usr->A);
    *bytes_A = malloc(*len_A);

    if (!*bytes_A) {
        *len_A = 0;
        *bytes_A = 0;
        *username = 0;
        return;
    }

    mbedtls_mpi_write_binary(usr->A, (unsigned char *) *bytes_A, *len_A);

    usr->bytes_A = *bytes_A;
    *username = usr->username;
}


/* Output: bytes_M. Buffer length is SHA512_DIGEST_LENGTH */
void srp_user_process_challenge(struct SRPUser *usr,
                                const unsigned char *bytes_s, int len_s,
                                const unsigned char *bytes_B, int len_B,
                                const unsigned char **bytes_M, int *len_M) {
    BIGNUM *s;
    BIGNUM *B;
    BIGNUM *u = 0;
    BIGNUM *x = 0;
    BIGNUM *k = 0;
    BIGNUM *v;
    BIGNUM *tmp1;
    BIGNUM *tmp2;
    BIGNUM *tmp3;

    s = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(s);
    mbedtls_mpi_read_binary(s, bytes_s, len_s);
    B = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(B);
    mbedtls_mpi_read_binary(B, bytes_B, len_B);
    v = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(v);
    tmp1 = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(tmp1);
    tmp2 = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(tmp2);
    tmp3 = (mbedtls_mpi *) malloc(sizeof(mbedtls_mpi));
    mbedtls_mpi_init(tmp3);

    *len_M = 0;
    *bytes_M = 0;

    if (!s || !B || !v || !tmp1 || !tmp2 || !tmp3)
        goto cleanup_and_exit;

    u = H_nn(usr->hash_alg, usr->A, B);

    if (!u)
        goto cleanup_and_exit;

    x = calculate_x(usr->hash_alg, s, usr->username, usr->password, usr->password_len);

    if (!x)
        goto cleanup_and_exit;

    k = H_nn(usr->hash_alg, usr->ng->N, usr->ng->g);

    if (!k)
        goto cleanup_and_exit;

    /* SRP-6a safety check */
    if (mbedtls_mpi_cmp_int(B, 0) != 0 && mbedtls_mpi_cmp_int(u, 0) != 0) {
        mbedtls_mpi_exp_mod(v, usr->ng->g, x, usr->ng->N, RR);
        /* S = (B - k*(g^x)) ^ (a + ux) */
        mbedtls_mpi_mul_mpi(tmp1, u, x);
        mbedtls_mpi_add_mpi(tmp2, usr->a, tmp1);
        /* tmp2 = (a + ux)      */
        mbedtls_mpi_exp_mod(tmp1, usr->ng->g, x, usr->ng->N, RR);
        mbedtls_mpi_mul_mpi(tmp3, k, tmp1);
        /* tmp3 = k*(g^x)       */
        mbedtls_mpi_sub_mpi(tmp1, B, tmp3);
        /* tmp1 = (B - K*(g^x)) */
        mbedtls_mpi_exp_mod(usr->S, tmp1, tmp2, usr->ng->N, RR);

        hash_num(usr->hash_alg, usr->S, usr->session_key);

        calculate_M(usr->hash_alg, usr->ng, usr->M, usr->username, s, usr->A, B, usr->session_key);
        calculate_H_AMK(usr->hash_alg, usr->H_AMK, usr->A, usr->M, usr->session_key);

        *bytes_M = usr->M;
        if (len_M) {
            *len_M = hash_length(usr->hash_alg);
        }
    } else {
        *bytes_M = NULL;
        if (len_M)
            *len_M = 0;
    }

    cleanup_and_exit:

    mbedtls_mpi_free(s);
    mbedtls_mpi_free(B);
    mbedtls_mpi_free(u);
    mbedtls_mpi_free(x);
    mbedtls_mpi_free(k);
    mbedtls_mpi_free(v);
    mbedtls_mpi_free(tmp1);
    mbedtls_mpi_free(tmp2);
    mbedtls_mpi_free(tmp3);
    free(s);
    free(B);
    free(u);
    free(x);
    free(k);
    free(v);
    free(tmp1);
    free(tmp2);
    free(tmp3);
}


void srp_user_verify_session(struct SRPUser *usr, const unsigned char *bytes_HAMK) {
    if (memcmp(usr->H_AMK, bytes_HAMK, hash_length(usr->hash_alg)) == 0)
        usr->authenticated = 1;
}

#undef init_random
