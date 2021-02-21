/* ---------------------------------------------------------------------------
Copyright:   (c) 2021 ff. Michael Amrhein (michael@adrhinum.de)
License:     This program is part of a larger application. For license
             details please read the file LICENSE.TXT provided together
             with the application.
------------------------------------------------------------------------------
$Source$
$Revision$
*/

#ifndef RATIONAL_PARSE_H
#define RATIONAL_PARSE_H

#include <Python.h>

#include "common.h"
#include "rn_fpdec.h"
#include "non_ascii_digits.h"

struct rn_parsed_repr {
    bool is_quot;
    bool neg;
    rn_exp_t exp;
    uint128_t coeff;
    uint64_t num;
    uint64_t den;
};

static inline error_t
invalid_literal() {
    PyErr_SetString(PyExc_ValueError, "Invalid literal for Rational.");
    return -1;
}

static inline int
map_to_dec_digit(Py_UCS4 uch) {
    if (uch >= '0' && uch <= '9')
        return (int)uch - '0';
    return lookup_non_ascii_digit(uch);
}

// parse a Rational literal
// [+/-]<num>/<den> or
// [+|-]<int>[.<frac>][<e|E>[+|-]<exp>] or
// [+|-].<frac>[<e|E>[+|-]<exp>].
static inline error_t
rn_from_ucs4_literal(struct rn_parsed_repr *parsed, Py_UCS4 *literal) {
    Py_UCS4 *cp = literal;
    uint128_t u128_accu = UINT128_ZERO;
    int64_t i64_accu = 0;
    Py_ssize_t n_dec_digits = 0;
    Py_ssize_t n_dec_int_digits;
    Py_ssize_t n_dec_frac_digits = 0;
    int d;
    bool leading_zero = false;

    for (; iswspace(*cp); ++cp);
    if (*cp == 0)
        return invalid_literal();
    parsed->neg = false;
    switch (*cp) {
        case '-':
            parsed->neg = true;
            FALLTHROUGH;
        case '+':
            ++cp;
    }
    while (*cp == '0' || lookup_non_ascii_digit(*cp) == '0') {
        leading_zero = true;
        ++cp;
    }
    while (*cp != 0 && (d = map_to_dec_digit(*cp)) >= 0) {
        if (n_dec_digits == UINT128_10_POW_N_CUTOFF)
            // there are more digits than coeff can hold, so give up
            return -1;
        u128_imul10_add_digit(&u128_accu, d);
        ++cp;
        ++n_dec_digits;
    }
    parsed->coeff = u128_accu;
    parsed->is_quot = false;
    switch (*cp) {
        case '.':
            ++cp;
            n_dec_int_digits = n_dec_digits;
            while (*cp != 0 && (d = map_to_dec_digit(*cp)) >= 0) {
                if (n_dec_digits == UINT128_10_POW_N_CUTOFF)
                    // there are more digits than coeff can hold, so give up
                    return -1;
                u128_imul10_add_digit(&u128_accu, d);
                ++n_dec_digits;
                ++cp;
            }
            parsed->coeff = u128_accu;
            n_dec_frac_digits = n_dec_digits - n_dec_int_digits;
            break;
        case '/':
            if (n_dec_digits > UINT64_10_POW_N_CUTOFF)
                // numerator overflowed, so give up
                return -1;
            ++cp;
            parsed->is_quot = true;
            parsed->num = U128_LO(u128_accu);
            n_dec_digits = 0;
            while (*cp != 0 && (d = map_to_dec_digit(*cp)) >= 0) {
                if (n_dec_digits > UINT64_10_POW_N_CUTOFF)
                    // denominator overflowed, so give up
                    return -1;
                i64_accu = i64_accu * 10 + d;
                ++n_dec_digits;
                ++cp;
            }
            parsed->den = i64_accu;
            break;
    }
    if (n_dec_digits == 0 && !leading_zero)
        return invalid_literal();
    if (!parsed->is_quot) {
        bool neg_exp = false;
        parsed->exp = 0;
        i64_accu = 0;
        if (*cp == 'e' || *cp == 'E') {
            ++cp;
            switch (*cp) {
                case '-':
                    neg_exp = true;
                    FALLTHROUGH;
                case '+':
                    ++cp;
            }
            d = map_to_dec_digit(*cp);
            if (d == -1)
                return invalid_literal();
            do {
                i64_accu = i64_accu * 10 + d;
                if (i64_accu > RN_MAX_EXP)
                    // exp overflowed, so give up
                    return -1;
                ++cp;
            } while (*cp != 0 && (d = map_to_dec_digit(*cp)) >= 0);
        }
        if (neg_exp) {
            i64_accu = -i64_accu - n_dec_frac_digits;
            if (i64_accu < RN_MIN_EXP)
                // exp overflowed, so give up
                return -1;
            parsed->exp = i64_accu;
        }
        else
            parsed->exp = (i64_accu - n_dec_frac_digits);
    }
    for (; iswspace(*cp); ++cp);
    if (*cp != 0)
        return invalid_literal();
    return 0;
}

#endif //RATIONAL_PARSE_H
