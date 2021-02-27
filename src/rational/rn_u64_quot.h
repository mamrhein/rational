/* ---------------------------------------------------------------------------
Copyright:   (c) 2021 ff. Michael Amrhein (michael@adrhinum.de)
License:     This program is part of a larger application. For license
             details please read the file LICENSE.TXT provided together
             with the application.
------------------------------------------------------------------------------
$Source$
$Revision$
*/

#ifndef RATIONAL_RN_U64_QUOT_H
#define RATIONAL_RN_U64_QUOT_H

#include <assert.h>

#include "rounding.h"
#include "uint64_math.h"

static inline uint64_t
gcd(uint64_t x, uint64_t y) {
    uint64_t t;
    while (x > 0) {
        if (x > y)
            x %= y;
        else {
            t = x;
            x = y % x;
            y = t;
        }
    }
    return y;
}

static inline void
rnq_reduce_quot(uint64_t *num, uint64_t *den) {
    uint64_t d = gcd(*num, *den);
    *num /= d;
    *den /= d;
}

static inline error_t
rnq_adjust_quot(uint64_t *num, uint64_t *den, bool neg, rn_prec_t to_prec) {
    unsigned int p = ABS(to_prec);
    uint128_t n, d;
    uint64_t t;

    if (p > UINT64_10_POW_N_CUTOFF)
        return -1;

    if (to_prec > 0) {
        t = u64_10_pow_n(p);
        u64_mul_u64(&n, *num, t);
        U128_FROM_LO_HI(&d, *den, 0ULL);
        u128_idiv_rounded(&n, &d, neg);
        p = u128_eliminate_trailing_zeros(&n, UINT64_10_POW_N_CUTOFF);
        t /= u64_10_pow_n(p);
        if (U128_HI(n) != 0)
            return -1;
        *num = U128_LO(n);
        *den = t;
        rnq_reduce_quot(num, den);
    }
    else if (to_prec < 0) {
        U128_FROM_LO_HI(&n, *num, 0ULL);
        t = u64_10_pow_n(p);
        u64_mul_u64(&d, *den, t);
        u128_idiv_rounded(&n, &d, neg);
        // n < 2^64, d > 1 => result < 2^64
        *num = U128_LO(n);
        *den = 1;
    }
    else {
        u64_idiv_rounded(num, *den, neg);
        *den = 1;
    }
    return 0;
}

static inline Py_ssize_t
rnq_magnitude(uint64_t num, uint64_t den) {
    return U64_MAGNITUDE(num) - U64_MAGNITUDE(den) - (num < den);
}

static inline PyObject *
rnq_to_int(rn_sign_t sign, uint64_t num, uint64_t den) {
    return PyLong_FromLongLong(sign * (int64_t)(num / den));
}

static inline PyObject *
rnq_to_float(rn_sign_t sign, uint64_t num, uint64_t den) {
    return PyFloat_FromDouble(sign * (double)num / den);
}

#endif //RATIONAL_RN_U64_QUOT_H
