/**
 * hapd
 *
 * Copyright 2018 Xule Zhou
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "common.h"

//Async math functions: enabling this makes computations longer, but avoid wdt triggers
#ifdef USE_ASYNC_MATH

/**
 * Make this async
 *
 * @warning In order to use this, you must manually export
 *  mpi_montg_init, mpi_montmul, mpi_montred in bignum.h
 *
 * @param result
 * @param base
 * @param exp
 * @param modulus
 * @param argument
 * @param callback
 * @param instance
 */
void hap_crypto_math_expmod(
        HAPServer * eventLoop,
        mbedtls_mpi * result,
        mbedtls_mpi * base,
        mbedtls_mpi * exp,
        mbedtls_mpi * modulus,
        void * argument,
        void (*callback)(void *, int)
);

void hap_crypto_math_init(HAPServer *);

#endif
