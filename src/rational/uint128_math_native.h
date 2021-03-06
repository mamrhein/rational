/* ---------------------------------------------------------------------------
Copyright:   (c) 2020 ff. Michael Amrhein (michael@adrhinum.de)
License:     This program is part of a larger application. For license
             details please read the file LICENSE.TXT provided together
             with the application.
------------------------------------------------------------------------------
$Source$
$Revision$
*/

#ifndef RATIONAL_UINT128_MATH_NATIVE_H
#define RATIONAL_UINT128_MATH_NATIVE_H

#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "uint64_math.h"

// Large unsigned int
typedef unsigned __int128 uint128_t;

static const uint128_t UINT128_ZERO = 0ULL;
static const uint128_t UINT128_ONE = 1ULL;
static const uint128_t UINT128_MAX =
    ((uint128_t)UINT64_MAX << 64U) + UINT64_MAX;

#define UINT128_10_POW_N_CUTOFF (2 * UINT64_10_POW_N_CUTOFF)

/*****************************************************************************
*  Macros
*****************************************************************************/

// properties
#define U128_MAGNITUDE(x) ((int) log10((double) x))

// byte assignment
#define U128_RHS(lo, hi) (((uint128_t)(hi) << 64U) + (lo))
#define U128_FROM_LO_HI(ui, lo, hi) *ui = U128_RHS((lo),( hi))

// byte selection
#define U128_HI(x) ((uint64_t)(((uint128_t)x) >> 64U))
#define U128_LO(x) ((uint64_t)(((uint128_t)x) & UINT64_MAX))
#define U128P_HI(x) U128_HI(*x)
#define U128P_LO(x) U128_LO(*x)

// tests
#define U128_EQ_ZERO(x) (x == 0)
#define U128_NE_ZERO(x) (x != 0)
#define U128P_EQ_ZERO(x) (*x == 0)
#define U128P_NE_ZERO(x) (*x != 0)

// overflow handling
#define SIGNAL_OVERFLOW(x) *x = UINT128_MAX
#define UINT128_CHECK_MAX(x) ((*x) == UINT128_MAX)

/*****************************************************************************
*  Functions
*****************************************************************************/

// Bit arithmetic

static inline unsigned
u128_n_signif_u32(const uint128_t x) {
    return (U128_HI(x) != 0) ?
           (U64_HI(U128_HI(x)) != 0 ? 4 : 3) :
           (U64_HI(U128_LO(x)) != 0 ? 2 : 1);
}

static inline bool
u128_is_uneven(const uint128_t *x) {
    return U128P_LO(x) & 1U;
}

static inline bool
u128_is_even(const uint128_t *x) {
    return !u128_is_uneven(x);
}

// Comparison

static inline int
u128_gt(const uint128_t x, const uint128_t y) {
    return x > y;
}

static inline int
u128_lt(const uint128_t x, const uint128_t y) {
    return x < y;
}

static inline int
u128_cmp(const uint128_t x, const uint128_t y) {
    return u128_gt(x, y) - u128_lt(x, y);
}

// Addition

static inline void
u128_iadd_u64(uint128_t *x, const uint64_t y) {
    *x += y;
}

static inline void
u128_incr(uint128_t *x) {
    (*x)++;
}

static inline void
u128_iadd_u128(uint128_t *x, const uint128_t *y) {
    *x += *y;
}

// Subtraction

static inline void
u128_isub_u64(uint128_t *x, const uint64_t y) {
    *x -= y;
}

static inline void
u128_decr(uint128_t *x) {
    (*x)--;
}

static inline void
u128_isub_u128(uint128_t *x, const uint128_t *y) {
    *x -= *y;
}

static inline void
u128_sub_u128(uint128_t *z, const uint128_t *x, const uint128_t *y) {
    *z = *x - *y;
}

// Multiplication

static inline void
u64_mul_u64(uint128_t *z, const uint64_t x, const uint64_t y) {
    *z = (uint128_t)x * y;
}

static inline void
u128_imul_u64(uint128_t *x, const uint64_t y) {
    if (U128_HI((uint128_t)U128P_HI(x) * y) != 0) {
        SIGNAL_OVERFLOW(x);
        return;
    }
    *x *= y;
}

static inline void
u128_imul_10_pow_n(uint128_t *x, const uint8_t n) {
    u128_imul_u64(x, u64_10_pow_n(n));
}

// Division

static inline uint64_t
u128_idiv_u32(uint128_t *x, uint32_t y) {
    assert(y != 0);
    uint128_t t = *x;
    *x /= y;
    return (uint64_t)(t - *x * y);
}

// Not really needed, but to be compatible with uint128_math.h
static inline uint64_t
u128_idiv_u64_special(uint128_t *x, uint64_t y) {
    assert(U64_HI(y) != 0);
    assert(U128P_HI(x) < y);
    uint128_t t = *x;
    *x /= y;
    return (uint64_t)(t - *x * y);
}

static inline uint64_t
u128_idiv_u64(uint128_t *x, const uint64_t y) {
    assert(y != 0);
    uint128_t t = *x;
    *x /= y;
    return (uint64_t)(t - *x * y);
}

// Not really needed, but to be compatible with uint128_math.h
static inline void
u128_idiv_u128_special(uint128_t *r, uint128_t *x, const uint128_t *y) {
    assert(U128P_HI(y) != 0);
    assert(u128_cmp(*x, *y) >= 0);
    uint128_t t = *x;
    *x /= *y;
    *r = t - *x * *y;
}

static inline void
u128_idiv_u128(uint128_t *r, uint128_t *x, const uint128_t *y) {
    uint128_t t = *x;
    *x /= *y;
    *r = t - *x * *y;
}

static inline uint64_t
u128_idiv_10(uint128_t *x) {
    uint128_t t = *x;
    *x /= 10;
    return t - *x * 10;
}

static inline uint128_t
u128_shift_right(const uint128_t *x, unsigned n_bits) {
    assert(n_bits < 64U);
    return *x >> n_bits;
}

static inline unsigned
u128_eliminate_trailing_zeros(uint128_t *x, unsigned n_max) {
    unsigned n_trailing_zeros = 0;

    while (*x != 0 && n_trailing_zeros < n_max && *x % 10U == 0) {
        *x /= 10U;
        n_trailing_zeros++;
    }
    return n_trailing_zeros;
}

static inline void
u128_imul10_add_digit(uint128_t *accu, int digit) {
    *accu = *accu * 10UL + digit;
}

#endif // RATIONAL_UINT128_MATH_H
