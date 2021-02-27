/* ---------------------------------------------------------------------------
Copyright:   (c) 2021 ff. Michael Amrhein (michael@adrhinum.de)
License:     This program is part of a larger application. For license
             details please read the file LICENSE.TXT provided together
             with the application.
------------------------------------------------------------------------------
$Source$
$Revision$
*/

#ifndef RATIONAL_RN_PYINT_QUOT_H
#define RATIONAL_RN_PYINT_QUOT_H

#include <Python.h>
#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "rounding.h"

// Python math functions
static PyObject *PyNumber_gcd = NULL;

// helper functions

static inline int
rnp_from_rational(PyIntQuot *rnp, PyObject *q) {
    ASSIGN_AND_CHECK_NULL(rnp->numerator,
                          PyObject_GetAttrString(q, "numerator"));
    ASSIGN_AND_CHECK_NULL(rnp->denominator,
                          PyObject_GetAttrString(q, "denominator"));
    return 0;

ERROR:
    Py_XDECREF(rnp->numerator);
    return -1;
}

static inline int
rnp_from_convertable(PyIntQuot *rnp, PyObject *num) {
    int rc = 0;
    PyObject *ratio = NULL;
    ASSIGN_AND_CHECK_NULL(ratio,
                          PyObject_CallMethod(num, "as_integer_ratio", NULL));
    ASSIGN_AND_CHECK_NULL(rnp->numerator, PySequence_GetItem(ratio, 0));
    ASSIGN_AND_CHECK_NULL(rnp->denominator, PySequence_GetItem(ratio, 1));
    goto CLEAN_UP;

ERROR:
    Py_XDECREF(rnp->numerator);
    rc = -1;

CLEAN_UP:
    Py_XDECREF(ratio);
    return rc;
}

static inline int
rnp_from_number(PyIntQuot *rnp, PyObject *num) {
    if (rnp_from_rational(rnp, num) == 0)
        return 0;
    return rnp_from_convertable(rnp, num);
}

// pre-condition: x > 0
static inline Py_ssize_t
pylong_magnitude(PyObject *x) {
    double d = PyLong_AsDouble(x);
    if (d == -1.0 && PyErr_Occurred()) {
        PyObject *s = NULL;
        Py_ssize_t n;
        PyErr_Clear();
        s = PyObject_Str(x);
        if (s == NULL)
            return -1;
        n = PyObject_Size(s);
        Py_DECREF(s);
        return n - 1;
    }
    double l = log10(d);
    return (Py_ssize_t)l;
}

static inline Py_ssize_t
rnp_magnitude(PyIntQuot *quot) {
    PyObject *abs_num = NULL;
    Py_ssize_t magn_num;
    Py_ssize_t magn_den;
    Py_ssize_t magn_quot;

    ASSIGN_AND_CHECK_NULL(abs_num, PyNumber_Absolute(quot->numerator));
    magn_num = pylong_magnitude(abs_num);
    if (magn_num == -1)
        goto ERROR;
    magn_den = pylong_magnitude(quot->denominator);
    if (magn_den == -1)
        goto ERROR;
    magn_quot = magn_num - magn_den -
                PyObject_RichCompareBool(abs_num, quot->denominator, Py_LT);
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());
    magn_quot = -1;

CLEAN_UP:
    Py_XDECREF(abs_num);
    return magn_quot;
}

static inline int
rnp_cmp(PyIntQuot *qx, PyIntQuot *qy) {
    int res = 0;
    int gt, lt;
    PyObject *lhs = NULL;
    PyObject *rhs = NULL;

    ASSIGN_AND_CHECK_NULL(lhs,
                          PyNumber_Multiply(qx->numerator, qy->denominator));
    ASSIGN_AND_CHECK_NULL(rhs,
                          PyNumber_Multiply(qy->numerator, qx->denominator));
    gt = PyObject_RichCompareBool(lhs, rhs, Py_GT);
    if (gt == -1)
        goto ERROR;
    lt = PyObject_RichCompareBool(lhs, rhs, Py_LT);
    if (lt == -1)
        goto ERROR;
    res = gt - lt;
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(lhs);
    Py_XDECREF(rhs);
    return res;
}

static inline error_t
rnp_reduce_inplace(PyIntQuot *rnp) {
    error_t rc = 0;
    PyObject *divisor = NULL;
    ASSIGN_AND_CHECK_NULL(divisor,
                          PyObject_CallFunctionObjArgs(PyNumber_gcd,
                                                       rnp->numerator,
                                                       rnp->denominator,
                                                       NULL));
    if (PyObject_RichCompareBool(divisor, PyONE, Py_NE)) {
        ASSIGN_AND_CHECK_NULL(rnp->numerator,
                              PyNumber_InPlaceFloorDivide(rnp->numerator,
                                                          divisor));
        ASSIGN_AND_CHECK_NULL(rnp->denominator,
                              PyNumber_InPlaceFloorDivide(rnp->denominator,
                                                          divisor));
    }
    goto CLEAN_UP;

ERROR:
    rc = -1;

CLEAN_UP:
    Py_XDECREF(divisor);
    return rc;
}

static PyObject *
rnp_div_rounded(PyObject *divident, PyObject *divisor) {
    // divisor must be >= 0 !!!
    enum RN_ROUNDING_MODE rounding_mode = rn_rounding_mode();
    PyObject *t = NULL;
    PyObject *q = NULL;
    PyObject *r = NULL;

    ASSIGN_AND_CHECK_NULL(t, PyNumber_Divmod(divident, divisor));
    ASSIGN_AND_CHECK_NULL(q, PySequence_GetItem(t, 0));
    ASSIGN_AND_CHECK_NULL(r, PySequence_GetItem(t, 1));

    if (PyObject_RichCompareBool(r, PyZERO, Py_EQ)) {
        // no need for rounding
        goto CLEAN_UP;
    }

    Py_CLEAR(t);
    switch (rounding_mode) {
        case RN_ROUND_05UP:
            // Round down unless last digit is 0 or 5
            // quotient not negativ and
            // quotient divisible by 5 without remainder or
            // quotient negativ and
            // (quotient + 1) not divisible by 5 without remainder
            // => add 1
            if (PyObject_RichCompareBool(q, PyZERO, Py_GE)) {
                ASSIGN_AND_CHECK_NULL(t, PyNumber_Remainder(q, PyFIVE));
                if (PyObject_RichCompareBool(t, PyZERO, Py_EQ))
                    q = PyNumber_InPlaceAdd(q, PyONE);
            }
            else {
                ASSIGN_AND_CHECK_NULL(t, PyNumber_Add(q, PyONE));
                ASSIGN_AND_CHECK_NULL(t, PyNumber_InPlaceRemainder(t, PyFIVE));
                if (PyObject_RichCompareBool(t, PyZERO, Py_NE))
                    q = PyNumber_InPlaceAdd(q, PyONE);
            }
            break;
        case RN_ROUND_CEILING:
            // Round towards Infinity (i. e. not towards 0 if non-negative)
            // => always add 1
            q = PyNumber_InPlaceAdd(q, PyONE);
            break;
        case RN_ROUND_DOWN:
            // Round towards 0 (aka truncate)
            // quotient negativ
            // => add 1
            if (PyObject_RichCompareBool(q, PyZERO, Py_LT))
                q = PyNumber_InPlaceAdd(q, PyONE);
            break;
        case RN_ROUND_FLOOR:
            // Round down (not towards 0 if negative)
            // => never add 1
            break;
        case RN_ROUND_HALF_DOWN:
            // Round 5 down (towards 0)
            // remainder > divisor/2 or
            // remainder = divisor/2 and quotient < 0
            // => add 1
            ASSIGN_AND_CHECK_NULL(t, PyNumber_Lshift(r, PyONE));
            if (PyObject_RichCompareBool(t, divisor, Py_GT) ||
                (PyObject_RichCompareBool(t, divisor, Py_EQ) &&
                 PyObject_RichCompareBool(q, PyZERO, Py_LT)))
                q = PyNumber_InPlaceAdd(q, PyONE);
            break;
        case RN_ROUND_HALF_EVEN:
            // Round 5 to even, rest to nearest
            // remainder > divisor/2 or
            // remainder = divisor/2 and quotient not even
            // => add 1
            ASSIGN_AND_CHECK_NULL(t, PyNumber_Lshift(r, PyONE));
            if (PyObject_RichCompareBool(t, divisor, Py_GT))
                q = PyNumber_InPlaceAdd(q, PyONE);
            else if (PyObject_RichCompareBool(t, divisor, Py_EQ)) {
                Py_CLEAR(t);
                ASSIGN_AND_CHECK_NULL(t, PyNumber_Remainder(q, PyTWO));
                if (PyObject_RichCompareBool(q, PyZERO, Py_NE))
                    q = PyNumber_InPlaceAdd(q, PyONE);
            }
            break;
        case RN_ROUND_HALF_UP:
            // Round 5 up (away from 0)
            // remainder > divisor/2 or
            // remainder = divisor/2 and quotient >= 0
            // => add 1
            ASSIGN_AND_CHECK_NULL(t, PyNumber_Lshift(r, PyONE));
            if (PyObject_RichCompareBool(t, divisor, Py_GT) ||
                (PyObject_RichCompareBool(t, divisor, Py_EQ) &&
                 PyObject_RichCompareBool(q, PyZERO, Py_GE)))
                q = PyNumber_InPlaceAdd(q, PyONE);
            break;
        case RN_ROUND_UP:
            // Round away from 0
            // quotient not negativ
            // => add 1
            if (PyObject_RichCompareBool(q, PyZERO, Py_GE))
                q = PyNumber_InPlaceAdd(q, PyONE);
            break;
        default:
            PyErr_SetString(PyExc_RuntimeError, "Unknown rounding mode");
            goto ERROR;
    }
    goto CLEAN_UP;

ERROR:
    Py_CLEAR(q);

CLEAN_UP:
    Py_XDECREF(t);
    Py_XDECREF(r);
    return q;
}

static int
rnp_adjusted(PyIntQuot *trgt, PyIntQuot *src, rn_prec_t to_prec) {
    int rc = 0;
    PyObject *t = NULL;
    PyObject *s = NULL;

    ASSIGN_AND_CHECK_NULL(t, PyLong_FromLong(ABS(to_prec)));
    ASSIGN_AND_CHECK_NULL(s, PyNumber_Power(PyTEN, t, Py_None));
    Py_CLEAR(t);
    if (to_prec >= 0) {
        ASSIGN_AND_CHECK_NULL(t, PyNumber_Multiply(src->numerator, s));
        ASSIGN_AND_CHECK_NULL(trgt->numerator,
                              rnp_div_rounded(t, src->denominator));
        Py_INCREF(s);
        trgt->denominator = s;
        if (rnp_reduce_inplace(trgt) == 0)
            goto CLEAN_UP;
    }
    else {
        ASSIGN_AND_CHECK_NULL(t, PyNumber_Multiply(src->denominator, s));
        ASSIGN_AND_CHECK_NULL(trgt->numerator, 
                              rnp_div_rounded(src->numerator, t));
        ASSIGN_AND_CHECK_NULL(trgt->numerator, 
                              PyNumber_InPlaceMultiply(trgt->numerator, s));
        Py_INCREF(PyONE);
        trgt->denominator = PyONE;
        goto CLEAN_UP;
    }

ERROR:
    rc = -1;

CLEAN_UP:
    Py_XDECREF(t);
    Py_XDECREF(s);
    return rc;
}

static inline PyObject *
rnp_to_int(PyIntQuot *q) {
    return PyNumber_FloorDivide(q->numerator, q->denominator);
}

static inline PyObject *
rnp_to_float(PyIntQuot *q) {
    return PyNumber_TrueDivide(q->numerator, q->denominator);
}

static inline int
rnp_add(PyIntQuot *res, PyIntQuot *qx, PyIntQuot *qy) {
    int rc = 0;
    PyObject *t1 = NULL;
    PyObject *t2 = NULL;

    assert(res->numerator == NULL);
    assert(res->denominator == NULL);

    ASSIGN_AND_CHECK_NULL(res->denominator,
                          PyNumber_Multiply(qx->denominator, qy->denominator));
    ASSIGN_AND_CHECK_NULL(t1,
                          PyNumber_Multiply(qx->numerator, qy->denominator));
    ASSIGN_AND_CHECK_NULL(t2,
                          PyNumber_Multiply(qy->numerator, qx->denominator));
    ASSIGN_AND_CHECK_NULL(res->numerator, PyNumber_Multiply(t1, t2));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());
    Py_CLEAR(res->numerator);
    Py_CLEAR(res->denominator);
    rc = -1;

CLEAN_UP:
    Py_XDECREF(t1);
    Py_XDECREF(t2);
    return rc;
}

#endif //RATIONAL_RN_PYINT_QUOT_H
