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

// maximum power of 10 less than UINT64_MAX (10 ^ UINT64_10_POW_N_CUTOFF)
#define MPT 10000000000000000000UL

/* Algorithm adopted from
 * Torbjörn Granlund and Peter L. Montgomery
 * Division by Invariant Integers using Multiplication
 * ACM SIGPLAN Notices, Vol. 29, No. 6
 *
 * The comments reference the formulas in the paper (translated to program
 * variables).
 *
 * N = 64
 * d = MPT = 10^19
 * l =  1 + ⌊log₂ d⌋ = 64
 * m' = ⌊(2^N ∗ (2^l − d) − 1) / d⌋ = 15581492618384294730
 * Since N = l:
 * dnorm = d << (N - l) = d
 * n2 = (HIGH(n) << (N - l)) + (LOW(n) >> l) = hi
 * n10 = LOW(n) << (N - l) = lo
 */
static inline uint64_t
u128_idiv_mpt_special(uint128_t *x) {
    static const uint64_t m = 15581492618384294730UL;
    uint128_t t = U128_RHS(m, 0);
    uint64_t lo = U128P_LO(x);
    uint64_t hi = U128P_HI(x);
    uint64_t neg_n1, n_adj, q1, q1_inv;

    // -n1 = XSIGN(n10) = n10 < 0 ? -1 : 0
    // neg_n1 = lo >= 2^63 ? UINT64_MAX : 0
    neg_n1 = -(lo >> 63U);
    // n_adj = n10 + AND(-n1, dnorm - 2^N)
    // i.e. n_adj = lo >= 2^63 ? lo + MPT : MPT
    n_adj = lo + (neg_n1 & MPT);
    // Estimation of q:
    // q1 = n2 + HIGH(m' * (n2 - (-n1)) + n_adj)
    // First step:
    // t = m' * (n2 - (-n1))
    // i.e. t = lo >= 2^63 ? m' * (hi + 1) : m' * hi
    u128_imul_u64(&t, hi - neg_n1);
    // Second step:
    // t = t + n_adj
    u128_iadd_u64(&t, n_adj);
    // Third step:
    // q1 = hi + HIGH(t)
    q1 = hi + U128_HI(t);
    // Now we have (see Lemma 8.1)
    // 0 <= q1 <= 2^N - 1
    // and
    // 0 <= n - q1 * d < 2 * d
    // or, in program terms
    // 0 <= q1 <= UINT64_MAX
    // and
    // 0 <= x - q1 * MPT < 2 * MPT
    // Thus, our upper estimation of r is n - q1 * d, i.e x - q1 * MPT.
    // The lower estimation of r can be calculated as
    // dr = n - q1 * d - d, or modulo int128
    // dr = n - 2^N * d + (2^N - 1 - q1) * d
    // Then
    // q = HIGH(dr) − (2^N − 1 − q1) + 2^N
    // r = LOW(dr) + AND(d − 2^N, HIGH(dr))
    // In program terms:
    // dr = x - q1 * MPT - MPT
    // dr = x - 2^64 * MPT + (UINT64_MAX - q1) * MPT
    // q = U128_HI(dr) − (UINT64_MAX - q1)
    // r = LOW(dr) + (MPT & U128_HI(dr))

    // With some short cuts:
    q1_inv = UINT64_MAX - q1;
    u64_mul_u64(&t, q1_inv, MPT);
    u128_iadd_u128(&t, x);
    hi = U128_HI(t) - MPT;
    U128_FROM_LO_HI(x, hi - q1_inv, 0);
    return U128_LO(t) + (MPT & hi);
}

static inline uint64_t
u128_idiv_mpt(uint128_t *x) {
    uint64_t lo = U128P_LO(x);
    uint64_t hi = U128P_HI(x);

    if (hi == 0) {
        if (lo < MPT) {
            U128_FROM_LO_HI(x, 0, 0);
            return lo;
        }
        else {
            U128_FROM_LO_HI(x, 1, 0);
            return lo - MPT;
        }
    }
    else
        return u128_idiv_mpt_special(x);
}

static inline PyObject *
rnd_to_str(const char *sign, uint128_t coeff, int exp) {
    PyObject *res = NULL;
    size_t n_dec_digits = U128_MAGNITUDE(coeff) + 1;
    size_t n_int_digits = n_dec_digits;
    size_t n_trailing_int_digits = 0;
    size_t n_leading_frac_digits = 0;
    size_t n_frac_digits = 0;
    size_t n_char;
    uint8_t *digits, *dp, *buf, *cp;
    uint8_t d;

    if (exp < 0) {
        unsigned abs_exp = ABS(exp);
        if (abs_exp < n_dec_digits) {
            n_frac_digits = abs_exp;
            n_int_digits = n_dec_digits - n_frac_digits;
        }
        else {
            n_int_digits = 0;
            n_trailing_int_digits = 1;
            n_frac_digits = n_dec_digits;
            n_leading_frac_digits = abs_exp - n_frac_digits;
        }
    }
    else if (exp > 0) {
        n_trailing_int_digits = exp;
    }
    n_char = n_int_digits + n_trailing_int_digits + n_leading_frac_digits +
        n_frac_digits + 3;
    buf = PyMem_Malloc(n_char);
    if (buf == NULL) {
        return PyErr_NoMemory();
    }
    dp = buf + n_char - 1;
    *dp = 0;
    while (U128_NE_ZERO(coeff)) {
        d = u128_idiv_10(&coeff);
        *(--dp) = '0' + d;
    }
    cp = buf;
    if (*sign != 0)
        *cp++ = *sign;
    for (size_t i = 0; i < n_int_digits; ++i) {
        *cp++ = *dp++;
    }
    for (size_t i = 0; i < n_trailing_int_digits; ++i)
        *cp++ = '0';
    if (n_frac_digits > 0) {
        *cp++ = '.';
        for (size_t i = 0; i < n_leading_frac_digits; ++i)
            *cp++ = '0';
        for (size_t i = 0; i < n_frac_digits; ++i) {
            *cp++ = *dp++;
        }
    }
    *cp = 0;
    res = PyUnicode_FromString((char *)buf);
    PyMem_Free(buf);
    return res;
}

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

static inline PyObject *
rnd_coeff_mul_10_pow_exp(uint128_t *coeff, rn_exp_t exp) {
    PyObject *res = NULL;
    PyObject *e = NULL;
    PyObject *t = NULL;

    ASSIGN_AND_CHECK_NULL(e, PyLong_FromLong(exp));
    ASSIGN_AND_CHECK_NULL(t, PyNumber_Power(PyTEN, e, Py_None));
    ASSIGN_AND_CHECK_NULL(res, pylong_from_u128(coeff));
    ASSIGN_AND_CHECK_NULL(res, PyNumber_InPlaceMultiply(res, t));

ERROR:
    assert (PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(e);
    Py_XDECREF(t);
    return res;
}

static inline PyObject *
rnd_coeff_floordiv_10_pow_exp(uint128_t *coeff, rn_exp_t exp) {
    PyObject *res = NULL;
    PyObject *e = NULL;
    PyObject *t = NULL;

    ASSIGN_AND_CHECK_NULL(e, PyLong_FromLong(exp));
    ASSIGN_AND_CHECK_NULL(t, PyNumber_Power(PyTEN, e, Py_None));
    ASSIGN_AND_CHECK_NULL(res, pylong_from_u128(coeff));
    ASSIGN_AND_CHECK_NULL(res, PyNumber_InPlaceFloorDivide(res, t));

ERROR:
    assert (PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(e);
    Py_XDECREF(t);
    return res;
}

static inline PyObject *
rnd_to_int(rn_sign_t sign, uint128_t coeff, rn_exp_t exp) {
    PyObject *res = NULL;
    PyObject *abs_res = NULL;
    int32_t abs_exp = ABS(exp);

    if (abs_exp < UINT64_10_POW_N_CUTOFF) {
        int32_t sh = u64_10_pow_n(abs_exp);
        if (exp < 0) {
            u128_idiv_u64(&coeff, sh);
            ASSIGN_AND_CHECK_NULL(abs_res, pylong_from_u128(&coeff));
        }
        else {
            uint128_t t = coeff;
            u128_imul_u64(&t, sh);
            if (u128_cmp(t, UINT128_MAX) == 0)
                ASSIGN_AND_CHECK_NULL(abs_res,
                                      rnd_coeff_mul_10_pow_exp(&coeff, exp));
            ASSIGN_AND_CHECK_NULL(abs_res, pylong_from_u128(&coeff));
        }
    }
    else if (exp < 0)
        ASSIGN_AND_CHECK_NULL(abs_res,
                              rnd_coeff_floordiv_10_pow_exp(&coeff, -exp));
    else
        ASSIGN_AND_CHECK_NULL(abs_res, rnd_coeff_mul_10_pow_exp(&coeff, exp));
    if (sign == RN_SIGN_NEG)
        ASSIGN_AND_CHECK_NULL(res, PyNumber_Negative(abs_res));
    else {
        Py_INCREF(abs_res);
        res = abs_res;
    }
    goto CLEAN_UP;

ERROR:
    assert (PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(abs_res);
    return res;
}

#endif //RATIONAL_RN_FPDEC_H
