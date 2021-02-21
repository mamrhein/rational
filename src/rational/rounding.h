/* ---------------------------------------------------------------------------
Copyright:   (c) 2021 ff. Michael Amrhein (michael@adrhinum.de)
License:     This program is part of a larger application. For license
             details please read the file LICENSE.TXT provided together
             with the application.
------------------------------------------------------------------------------
$Source$
$Revision$
*/

#ifndef RATIONAL_ROUNDING_H
#define RATIONAL_ROUNDING_H

#include <Python.h>

#include "common.h"
#ifdef __SIZEOF_INT128__
#include "uint128_math_native.h"
#else
#include "uint128_math.h"
#endif // __int128


enum RN_ROUNDING_MODE {
    // Round away from zero if last digit after rounding towards
    // zero would have been 0 or 5; otherwise round towards zero.
    RN_ROUND_05UP = 1,
    // Round towards Infinity.
    RN_ROUND_CEILING = 2,
    // Round towards zero.
    RN_ROUND_DOWN = 3,
    // Round towards -Infinity.
    RN_ROUND_FLOOR = 4,
    // Round to nearest with ties going towards zero.
    RN_ROUND_HALF_DOWN = 5,
    // Round to nearest with ties going to nearest even integer.
    RN_ROUND_HALF_EVEN = 6,
    // Round to nearest with ties going away from zero.
    RN_ROUND_HALF_UP = 7,
    // Round away from zero.
    RN_ROUND_UP = 8,
};

// will be imported from rounding.py
static PyObject *Rounding = NULL;
static PyObject *get_dflt_rounding_mode = NULL;

static enum RN_ROUNDING_MODE
rn_rounding_mode() {
    long rounding_mode = 0;
    PyObject *dflt = NULL;
    PyObject *val = NULL;

#if PY_VERSION_HEX >= 0x03090000
    ASSIGN_AND_CHECK_NULL(dflt, PyObject_CallNoArgs(get_dflt_rounding_mode));
#else
    ASSIGN_AND_CHECK_NULL(dflt,
                          PyObject_CallObject(get_dflt_rounding_mode, NULL));
#endif
    ASSIGN_AND_CHECK_NULL(val, PyObject_GetAttrString(dflt, "value"));
    rounding_mode = PyLong_AsLong(val);

ERROR:
    Py_XDECREF(dflt);
    Py_XDECREF(val);
    return (enum RN_ROUNDING_MODE)rounding_mode;
}

static void
u64_idiv_rounded(uint64_t *divident, uint64_t divisor, bool neg) {
    enum RN_ROUNDING_MODE rounding = rn_rounding_mode();
    uint64_t quot, rem, tie;

    rem = *divident % divisor;
    *divident /= divisor;
    quot = *divident;
    switch (rounding) {
        case RN_ROUND_05UP:
            // Round down unless last digit is 0 or 5
            if (quot % 5 == 0)
                ++(*divident);
            break;
        case RN_ROUND_CEILING:
            // Round towards Infinity (i. e. not towards 0 if non-negative)
            if (!neg)
                ++(*divident);
            break;
        case RN_ROUND_DOWN:
            // Round towards 0 (aka truncate)
            break;
        case RN_ROUND_FLOOR:
            // Round towards -Infinity (i.e. not towards 0 if negative)
            if (neg)
                ++(*divident);
            break;
        case RN_ROUND_HALF_DOWN:
            // Round 5 down, rest to nearest
            tie = divisor >> 1U;
            if (rem > tie)
                ++(*divident);
            break;
        case RN_ROUND_HALF_EVEN:
            // Round 5 to nearest even, rest to nearest
            tie = divisor >> 1U;
            if (rem > tie || (rem == tie && u64_is_even(divisor) &&
                              u64_is_uneven(quot)))
                ++(*divident);
            break;
        case RN_ROUND_HALF_UP:
            // Round 5 up (away from 0), rest to nearest
            tie = divisor >> 1U;
            if (rem > tie || (rem == tie && u64_is_even(divisor)))
                ++(*divident);
            break;
        case RN_ROUND_UP:
            // Round away from 0
            ++(*divident);
            break;
        default:
            break;
    }
}

static void
u128_idiv_rounded(uint128_t *divident, const uint128_t *divisor, bool neg) {
    enum RN_ROUNDING_MODE rounding = rn_rounding_mode();
    uint128_t quot, rem, tie;
    int cmp;

    u128_idiv_u128(&rem, divident, divisor);
    quot = *divident;
    switch (rounding) {
        case RN_ROUND_05UP:
            // Round down unless last digit is 0 or 5
            if (u128_idiv_u32(&quot, 5) == 0)
                u128_incr(divident);
            break;
        case RN_ROUND_CEILING:
            // Round towards Infinity (i. e. not towards 0 if non-negative)
            if (!neg)
                u128_incr(divident);
            break;
        case RN_ROUND_DOWN:
            // Round towards 0 (aka truncate)
            break;
        case RN_ROUND_FLOOR:
            // Round towards -Infinity (i.e. not towards 0 if negative)
            if (neg)
                u128_incr(divident);
            break;
        case RN_ROUND_HALF_DOWN:
            // Round 5 down, rest to nearest
            tie = u128_shift_right(divisor, 1UL);
            if (u128_cmp(rem, tie) > 0)
                u128_incr(divident);
            break;
        case RN_ROUND_HALF_EVEN:
            // Round 5 to nearest even, rest to nearest
            tie = u128_shift_right(divisor, 1UL);
            cmp = u128_cmp(rem, tie);
            if (cmp > 0 || (cmp == 0 && u128_is_even(divisor) &&
                            u128_is_uneven(&quot)))
                u128_incr(divident);
            break;
        case RN_ROUND_HALF_UP:
            // Round 5 up (away from 0), rest to nearest
            tie = u128_shift_right(divisor, 1UL);
            cmp = u128_cmp(rem, tie);
            if (cmp > 0 || (cmp == 0 && u128_is_even(divisor)))
                u128_incr(divident);
            break;
        case RN_ROUND_UP:
            // Round away from 0
            u128_incr(divident);
            break;
        default:
            break;
    }
}

#endif //RATIONAL_ROUNDING_H
