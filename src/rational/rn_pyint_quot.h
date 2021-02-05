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
#include <math.h>

#include "common.h"

static inline int
rnp_from_rational(PyIntQuot *rnp, PyObject *num) {
    ASSIGN_AND_CHECK_NULL(rnp->numerator,
                          PyObject_GetAttrString(num, "numerator"));
    ASSIGN_AND_CHECK_NULL(rnp->denominator,
                          PyObject_GetAttrString(num, "denominator"));
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
static inline PyObject *
pylong_floor_log10(PyObject *x) {
    double d = PyLong_AsDouble(x);
    if (d == -1.0 && PyErr_Occurred())
        return NULL;
    double l = log10(d);
    return PyLong_FromDouble(l);
}

static inline PyObject *
rnp_magnitude(PyIntQuot *quot) {
    PyObject *res = NULL;
    PyObject *abs_num = NULL;
    PyObject *magn_num = NULL;
    PyObject *magn_den = NULL;

    ASSIGN_AND_CHECK_NULL(abs_num, PyNumber_Absolute(quot->numerator));
    ASSIGN_AND_CHECK_NULL(magn_num, pylong_floor_log10(abs_num));
    ASSIGN_AND_CHECK_NULL(magn_den, pylong_floor_log10(quot->denominator));
    ASSIGN_AND_CHECK_NULL(res, PyNumber_Subtract(magn_num, magn_den));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(abs_num);
    Py_XDECREF(magn_num);
    Py_XDECREF(magn_den);
    return res;
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
