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
u64_idiv_rounded(uint64_t *divident, const uint64_t *divisor, bool neg) {

}

static void
u128_idiv_rounded(uint128_t *divident, const uint128_t *divisor, bool neg) {

}

#endif //RATIONAL_ROUNDING_H
