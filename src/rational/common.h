/* ---------------------------------------------------------------------------
Copyright:   (c) 2021 ff. Michael Amrhein (michael@adrhinum.de)
License:     This program is part of a larger application. For license
             details please read the file LICENSE.TXT provided together
             with the application.
------------------------------------------------------------------------------
$Source$
$Revision$
*/

#ifndef RATIONAL_COMMON_H
#define RATIONAL_COMMON_H

// sign indicator: 0 -> zero, -1 -> negative, 1 -> positive
typedef int8_t rn_sign_t;
#define RN_SIGN_ZERO 0
#define RN_SIGN_NEG -1
#define RN_SIGN_POS 1

// exponent of internal representation
typedef int32_t rn_exp_t;
#define RN_UNLIM_EXP INT32_MIN
#define RN_MIN_EXP (INT32_MIN + 1)
#define RN_MAX_EXP INT32_MAX

// number of decimal fractional digits (function param)
typedef int32_t rn_prec_t;
#define RN_UNLIM_PREC RN_UNLIM_EXP
#define RN_MIN_PREC -RN_MAX_EXP
#define RN_MAX_PREC -RN_MIN_EXP

// Python int numerator / denominator
typedef struct pyint_quot {
    PyObject *numerator;
    PyObject *denominator;
} PyIntQuot;

// Macros to simplify error checking

#define ASSIGN_AND_CHECK_NULL(result, expr) \
    do { result = (expr); if (result == NULL) goto ERROR; } while (0)

#define ASSIGN_AND_CHECK_OK(result, expr) \
    do { result = (expr); if (result != NULL) goto CLEAN_UP; } while (0)

#define CHECK_RC(expr) if ((expr) != 0) goto ERROR

#define CHECK_TYPE(obj, type) \
    if (!PyObject_TypeCheck(obj, (PyTypeObject *)type)) goto ERROR

#endif //RATIONAL_COMMON_H
