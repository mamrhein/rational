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
typedef int16_t rn_exp_t;
#define RN_MAX_EXP INT16_MAX
#define RN_MIN_EXP (-INT16_MAX)
#define RN_UNDEF_EXP INT16_MIN

// number of decimal fractional digits
typedef int16_t rn_prec_t;
#define RN_UNLIM_PREC INT16_MAX
#define RN_MAX_PREC 9999
#define RN_MIN_PREC (-RN_MAX_PREC)

// Python int numerator / denominator
typedef struct pyint_quot {
    PyObject *numerator;
    PyObject *denominator;
} PyIntQuot;

// PyLong methods
static PyObject *PyLong_bit_length = NULL;

// Python number constants
static PyObject *PyZERO = NULL;
static PyObject *PyONE = NULL;
static PyObject *PyTWO = NULL;
static PyObject *PyFIVE = NULL;
static PyObject *PyTEN = NULL;
static PyObject *Py64 = NULL;
static PyObject *PyUInt64Max = NULL;
static PyObject *Py2pow64 = NULL;

// Macros to simplify error checking

#define ASSIGN_AND_CHECK_NULL(result, expr) \
    do { result = (expr); if (result == NULL) goto ERROR; } while (0)

#define ASSIGN_AND_CHECK_OK(result, expr) \
    do { result = (expr); if (result != NULL) goto CLEAN_UP; } while (0)

#define CHECK_RC(expr) if ((expr) != 0) goto ERROR

#define CHECK_TYPE(obj, type) \
    if (!PyObject_TypeCheck(obj, (PyTypeObject *)type)) goto ERROR

#endif //RATIONAL_COMMON_H
