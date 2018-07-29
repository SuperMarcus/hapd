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
