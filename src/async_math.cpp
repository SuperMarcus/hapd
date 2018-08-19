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

/**
 * Modified mbedtls mpi exp function
 */

#include <cstring>
#include "common.h"

#ifdef USE_ASYNC_MATH

#ifndef ciL
#define ciL    (sizeof(mbedtls_mpi_uint))         /* chars in limb  */
#endif

#ifndef biL
#define biL    (ciL << 3)               /* bits  in limb  */
#endif

#ifndef biH
#define biH    (ciL << 2)               /* half limb size */
#endif

#ifndef MPI_SIZE_T_MAX
#define MPI_SIZE_T_MAX  ( (size_t) -1 ) /* SIZE_T_MAX is not standard */
#endif

#include "hap_crypto.h"
#include "HomeKitAccessory.h"

#include "crypto/ctr_drbg.h"
#include "crypto/bignum.h"
#include "crypto/bn_mul.h"
#include "crypto/platform.h"
#include "crypto/srp.h"

#include "async_math.h"

struct _async_math_expmod_info {
    HAPServer *loop;
    mbedtls_mpi *X;
    mbedtls_mpi *A;
    mbedtls_mpi *E;
    mbedtls_mpi *N;
    void *argument;
    void (*callback)(void *, int);

    int ret, neg;
    size_t wbits, wsize, one = 1;
    size_t i, j, nblimbs;
    size_t bufsize, nbits;
    mbedtls_mpi_uint ei, mm, state;
    mbedtls_mpi RR, * _RR, T, W[ 2 << MBEDTLS_MPI_WINDOW_SIZE ], Apos;
};

void _async_math_expmod_cleanup(_async_math_expmod_info * info){
    for( info->i = ( info->one << ( info->wsize - 1 ) ); info->i < ( info->one << info->wsize ); info->i++ )
        mbedtls_mpi_free( &info->W[info->i] );

    mbedtls_mpi_free( &info->W[1] ); mbedtls_mpi_free( &info->T ); mbedtls_mpi_free( &info->Apos );

    if( info->_RR == NULL || info->_RR->p == NULL )
        mbedtls_mpi_free( &info->RR );

    auto cb = info->callback;
    cb(info->argument, info->ret);

    delete info;
}

void _async_math_expmod_final(HAPEvent * event){
    auto info = event->arg<_async_math_expmod_info>();
    auto& ret = info->ret;

    /*
     * process the remaining bits
     */
    for( info->i = 0; info->i < info->nbits; info->i++ )
    {
        MBEDTLS_MPI_CHK( mpi_montmul( info->X, info->X, info->N, info->mm, &info->T ) );

        info->wbits <<= 1;

        if( ( info->wbits & ( info->one << info->wsize ) ) != 0 )
            MBEDTLS_MPI_CHK( mpi_montmul( info->X, &info->W[1], info->N, info->mm, &info->T ) );
    }

    /*
     * X = A^E * R * R^-1 mod N = A^E mod N
     */
    MBEDTLS_MPI_CHK( mpi_montred( info->X, info->N, info->mm, &info->T ) );

    if( info->neg && info->E->n != 0 && ( info->E->p[0] & 1 ) != 0 )
    {
        info->X->s = -1;
        MBEDTLS_MPI_CHK( mbedtls_mpi_add_mpi( info->X, info->N, info->X ) );
    }

    cleanup:
    _async_math_expmod_cleanup(info);
}

void _async_math_expmod_body(HAPEvent * event){
    auto info = event->arg<_async_math_expmod_info>();
    auto& ret = info->ret;

    if( info->bufsize == 0 )
    {
        if( info->nblimbs == 0 )
            goto _break;

        info->nblimbs--;

        info->bufsize = sizeof( mbedtls_mpi_uint ) << 3;
    }

    info->bufsize--;

    info->ei = (info->E->p[info->nblimbs] >> info->bufsize) & 1;

    /*
     * skip leading 0s
     */
    if( info->ei == 0 && info->state == 0 )
        goto _continue;

    if( info->ei == 0 && info->state == 1 )
    {
        /*
         * out of window, square X
         */
        MBEDTLS_MPI_CHK( mpi_montmul( info->X, info->X, info->N, info->mm, &info->T ) );
        goto _continue;
    }

    /*
     * add ei to current window
     */
    info->state = 2;

    info->nbits++;
    info->wbits |= ( info->ei << ( info->wsize - info->nbits ) );

    if( info->nbits == info->wsize )
    {
        /*
         * X = X^wsize R^-1 mod N
         */
        for( info->i = 0; info->i < info->wsize; info->i++ )
            MBEDTLS_MPI_CHK( mpi_montmul( info->X, info->X, info->N, info->mm, &info->T ) );

        /*
         * X = X * W[wbits] R^-1 mod N
         */
        MBEDTLS_MPI_CHK( mpi_montmul( info->X, &info->W[info->wbits], info->N, info->mm, &info->T ) );

        info->state--;
        info->nbits = 0;
        info->wbits = 0;
    }

    _continue:
    info->loop->emit(HAPEvent::HAPCRYPTO_ASYNC_EXPMOD_BODY, info);
    return;

    _break:
    info->loop->emit(HAPEvent::HAPCRYPTO_ASYNC_EXPMOD_FINAL, info);
    return;

    cleanup:
    _async_math_expmod_cleanup(info);
}

void hap_crypto_math_expmod(
        HAPServer *eventLoop,
        mbedtls_mpi *X,
        mbedtls_mpi *A,
        mbedtls_mpi *E,
        mbedtls_mpi *N,
        void *argument,
        void (*callback)(void *, int)) {

    auto info = new _async_math_expmod_info;
    auto& ret = info->ret;

    info->loop = eventLoop;
    info->X = X;
    info->A = A;
    info->E = E;
    info->N = N;
    info->_RR = csrp_speed_RR();

    info->argument = argument;
    info->callback = callback;

    if( mbedtls_mpi_cmp_int( N, 0 ) <= 0 || ( N->p[0] & 1 ) == 0 )
        return;

    if( mbedtls_mpi_cmp_int( E, 0 ) < 0 )
        return;

    /*
     * Init temps and window size
     */
    mpi_montg_init( &info->mm, N );
    mbedtls_mpi_init( &info->RR ); mbedtls_mpi_init( &info->T );
    mbedtls_mpi_init( &info->Apos );
    memset( info->W, 0, sizeof( info->W ) );

    info->i = mbedtls_mpi_bitlen( E );

    info->wsize = ( info->i > 671 ) ? 6 : ( info->i > 239 ) ? 5 :
                              ( info->i >  79 ) ? 4 : ( info->i >  23 ) ? 3 : 1;

    if( info->wsize > MBEDTLS_MPI_WINDOW_SIZE )
        info->wsize = MBEDTLS_MPI_WINDOW_SIZE;

    info->j = N->n + 1;
    MBEDTLS_MPI_CHK( mbedtls_mpi_grow( X, info->j ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_grow( &info->W[1],  info->j ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_grow( &info->T, info->j * 2 ) );

    /*
     * Compensate for negative A (and correct at the end)
     */
    info->neg = ( A->s == -1 );
    if( info->neg )
    {
        MBEDTLS_MPI_CHK( mbedtls_mpi_copy( &info->Apos, A ) );
        info->Apos.s = 1;
        A = &info->Apos;
    }

    /*
     * If 1st call, pre-compute R^2 mod N
     */
    if(info->_RR == NULL || info->_RR->p == NULL )
    {
        MBEDTLS_MPI_CHK( mbedtls_mpi_lset( &info->RR, 1 ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_shift_l( &info->RR, N->n * 2 * biL ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &info->RR, &info->RR, N ) );

        if( info->_RR != NULL )
            memcpy( info->_RR, &info->RR, sizeof( mbedtls_mpi ) );
    }
    else
        memcpy( &info->RR, info->_RR, sizeof( mbedtls_mpi ) );

    /*
     * W[1] = A * R^2 * R^-1 mod N = A * R mod N
     */
    if( mbedtls_mpi_cmp_mpi( A, N ) >= 0 )
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &info->W[1], A, N ) );
    else
        MBEDTLS_MPI_CHK( mbedtls_mpi_copy( &info->W[1], A ) );

    MBEDTLS_MPI_CHK( mpi_montmul( &info->W[1], &info->RR, N, info->mm, &info->T ) );

    /*
     * X = R^2 * R^-1 mod N = R mod N
     */
    MBEDTLS_MPI_CHK( mbedtls_mpi_copy( X, &info->RR ) );
    MBEDTLS_MPI_CHK( mpi_montred( X, N, info->mm, &info->T ) );

    if( info->wsize > 1 )
    {
        /*
         * W[1 << (wsize - 1)] = W[1] ^ (wsize - 1)
         */
        info->j =  info->one << ( info->wsize - 1 );

        MBEDTLS_MPI_CHK( mbedtls_mpi_grow( &info->W[info->j], N->n + 1 ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_copy( &info->W[info->j], &info->W[1]    ) );

        for( info->i = 0; info->i < info->wsize - 1; info->i++ )
            MBEDTLS_MPI_CHK( mpi_montmul( &info->W[info->j], &info->W[info->j], N, info->mm, &info->T ) );

        /*
         * W[i] = W[i - 1] * W[1]
         */
        for( info->i = info->j + 1; info->i < ( info->one << info->wsize ); info->i++ )
        {
            MBEDTLS_MPI_CHK( mbedtls_mpi_grow( &info->W[info->i], N->n + 1 ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_copy( &info->W[info->i], &info->W[info->i - 1] ) );

            MBEDTLS_MPI_CHK( mpi_montmul( &info->W[info->i], &info->W[1], N, info->mm, &info->T ) );
        }
    }

    info->nblimbs = E->n;
    info->bufsize = 0;
    info->nbits   = 0;
    info->wbits   = 0;
    info->state   = 0;

    info->loop->emit(HAPEvent::HAPCRYPTO_ASYNC_EXPMOD_BODY, info);
    return;
    cleanup: _async_math_expmod_cleanup(info);
}

void hap_crypto_math_init(HAPServer * loop) {
    loop->on(HAPEvent::HAPCRYPTO_ASYNC_EXPMOD_BODY, _async_math_expmod_body);
    loop->on(HAPEvent::HAPCRYPTO_ASYNC_EXPMOD_FINAL, _async_math_expmod_final);
}

#endif
