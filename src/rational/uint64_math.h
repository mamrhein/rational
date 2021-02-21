/* ---------------------------------------------------------------------------
Copyright:   (c) 2020 ff. Michael Amrhein (michael@adrhinum.de)
License:     This program is part of a larger application. For license
             details please read the file LICENSE.TXT provided together
             with the application.
------------------------------------------------------------------------------
$Source$
$Revision$
*/

#ifndef RATIONAL_UINT64_MATH_H
#define RATIONAL_UINT64_MATH_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

/*****************************************************************************
*  Macros
*****************************************************************************/

// byte selection
#define U64_HI(x) (((uint64_t)x) >> 32U)
#define U64_LO(x) (((uint64_t)x) & 0xFFFFFFFF)

// int compare
#define CMP(a, b) (((a) > (b)) - ((a) < (b)))

// abs / max / min
#define ABS(a) ((a) >= 0 ? (a) : (-a))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

// modulo arith
#define CEIL(a, b) (((a) % (b)) <= 0 ? (a) / (b) : (a) / (b) + 1)
#define FLOOR(a, b) (((a) % (b)) < 0 ? (a) / (b) - 1 : (a) / (b))
#define MOD(a, b) ((a) - FLOOR((a), (b)) * (b))

// properties
#define U64_MAGNITUDE(x) ((int) log10(x))

/*****************************************************************************
*  Functions
*****************************************************************************/

// Bit arithmetic

static inline unsigned
u64_most_signif_bit_pos(uint64_t x) {
    unsigned n = 0;
    uint64_t t;

    for (unsigned shift = 32U; shift > 0; shift >>= 1U) {
        t = x >> shift;
        if (t != 0) {
            n += shift;
            x = t;
        }
    }
    return n;
}

static inline unsigned
u64_n_leading_0_bits(const uint64_t x) {
    if (x == 0) return 64;
    return 63 - u64_most_signif_bit_pos(x);
}

static inline bool
u64_is_uneven(const uint64_t x) {
    return x & 1U;
}

static inline bool
u64_is_even(const uint64_t x) {
    return !u64_is_uneven(x);
}

// powers of 10
#define UINT64_10_POW_N_CUTOFF 19
static uint64_t U64_10_POWS[20] = {
    1UL,
    10UL,
    100UL,
    1000UL,
    10000UL,
    100000UL,
    1000000UL,
    10000000UL,
    100000000UL,
    1000000000UL,
    10000000000UL,
    100000000000UL,
    1000000000000UL,
    10000000000000UL,
    100000000000000UL,
    1000000000000000UL,
    10000000000000000UL,
    100000000000000000UL,
    1000000000000000000UL,
    10000000000000000000UL
};

static inline uint64_t
u64_10_pow_n(unsigned int exp) {
    assert(exp <= UINT64_10_POW_N_CUTOFF);
    return U64_10_POWS[exp];
}

#endif //RATIONAL_UINT64_MATH_H
