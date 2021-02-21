/* ---------------------------------------------------------------------------
Copyright:   (c) 2021 ff. Michael Amrhein (michael@adrhinum.de)
License:     This program is part of a larger application. For license
             details please read the file LICENSE.TXT provided together
             with the application.
------------------------------------------------------------------------------
$Source$
$Revision$
*/

#ifndef RATIONAL_RN_FPDEC_H
#define RATIONAL_RN_FPDEC_H

#include <Python.h>

#ifdef __SIZEOF_INT128__
#include "uint128_math_native.h"
#else
#include "uint128_math.h"
#endif // __int128

#include "rounding.h"

static inline PyObject *
PyLong_from_u128_lo_hi(uint64_t lo, uint64_t hi) {
    PyObject *res = NULL;
    PyObject *res_hi = NULL;
    PyObject *res_lo = NULL;
    PyObject *t = NULL;

    ASSIGN_AND_CHECK_NULL(res_hi, PyLong_FromUnsignedLongLong(hi));
    ASSIGN_AND_CHECK_NULL(res_lo, PyLong_FromUnsignedLongLong(lo));
    ASSIGN_AND_CHECK_NULL(t, PyNumber_Lshift(res_hi, Py64));
    ASSIGN_AND_CHECK_NULL(res, PyNumber_Add(t, res_lo));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(res_hi);
    Py_XDECREF(res_lo);
    Py_XDECREF(t);
    return res;
}

static inline PyObject *
pylong_from_u128(const uint128_t *ui) {
    if (U128P_HI(ui) == 0)
        return PyLong_FromUnsignedLongLong(U128P_LO(ui));
    else
        return PyLong_from_u128_lo_hi(U128P_LO(ui), U128P_HI(ui));
}

static inline uint64_t
two_pow_n(uint32_t n) {
    return (1ULL << n);
}

static inline uint64_t
five_pow_n(uint32_t n) {
    uint64_t b = 5;
    uint64_t r = 1;
    while (n) {
        if (n % 2)
            r *= b;
        n >>= 1;
        b *= b;
    }
    return r;
}

static inline int32_t
least_pow_10_multiple(uint64_t *factor, uint64_t n) {
    uint32_t nf2 = 0;
    uint32_t nf5 = 0;
    uint32_t nf10 = 0;
    while (n >= 10 && (n % 10) == 0) {
        n /= 10;
        ++nf10;
    }
    while (n >= 5 && (n % 5) == 0) {
        n /= 5;
        ++nf5;
    }
    while (n >= 2 && (n % 2) == 0) {
        n /= 2;
        ++nf2;
    }
    if (n == 1) {
        uint32_t t;
        if (nf2 > nf5) {
            t = nf2 - nf5;
            if (t > 27)
                // result would overflow
                return -1;
            *factor = five_pow_n(t);
            return nf10 + nf2;
        }
        else if (nf5 > nf2) {
            t = nf5 - nf2;
            if (t > 63)
                // result would overflow
                return -1;
            *factor = two_pow_n(t);
            return nf10 + nf5;
        }
        else {
            *factor = 1;
            return nf10 + nf2;
        }
    }
    return -1;
}

static inline error_t
rnd_from_quot(uint128_t *coeff, rn_exp_t *exp, uint64_t num, uint64_t den) {
    uint64_t factor;
    int32_t m = least_pow_10_multiple(&factor, den);
    if (m < 0)
        return -1;
    *exp = -m;
    u64_mul_u64(coeff, num, factor);
    return 0;
}

static inline int
rnd_magnitude(uint128_t coeff, rn_exp_t exp) {
    if (U128_HI(coeff) == 0)
        return U64_MAGNITUDE(U128_LO(coeff)) + exp;
    else
        return U128_MAGNITUDE(coeff) + exp;
}

static inline error_t
rnd_adjust_coeff_exp(uint128_t *coeff, rn_exp_t *exp, bool neg,
                     rn_prec_t to_prec) {
    int sh = -(to_prec + *exp);

    if (sh > UINT64_10_POW_N_CUTOFF)
        return -1;

    if (sh > 0) {
        uint128_t t;
        U128_FROM_LO_HI(&t, u64_10_pow_n(sh), 0ULL);
        u128_idiv_rounded(coeff, &t, neg);
        *exp = -to_prec;
    }
    return 0;
}

#endif //RATIONAL_RN_FPDEC_H
