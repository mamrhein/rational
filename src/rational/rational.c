/* ---------------------------------------------------------------------------
Copyright:   (c) 2021 ff. Michael Amrhein (michael@adrhinum.de)
License:     This program is part of a larger application. For license
             details please read the file LICENSE.TXT provided together
             with the application.
------------------------------------------------------------------------------
$Source$
$Revision$
*/

#undef __SIZEOF_INT128__
#define PY_SSIZE_T_CLEAN
#define Py_LIMITED_API 0x03070000

#include <Python.h>
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include "common.h"
#include "compiler_macros.h"
#include "docstrings.h"
#include "parse.h"
#include "rn_fpdec.h"
#include "rn_pyint_quot.h"
#include "rn_u64_quot.h"
#include "rounding.h"


// error handling

static inline PyObject *
value_error_ptr(const char *msg) {
    PyErr_SetString(PyExc_ValueError, msg);
    return NULL;
}

static inline PyObject *
type_error_ptr(const char *msg) {
    PyErr_SetString(PyExc_TypeError, msg);
    return NULL;
}

// Abstract number types

static PyObject *Number = NULL;
static PyObject *Complex = NULL;
static PyObject *Real = NULL;
static PyObject *Rational = NULL;
static PyObject *Integral = NULL;

// Concrete number types

static PyObject *Fraction = NULL;
static PyObject *Decimal = NULL;

// Python constants for hash function

static PyObject *PyHASH_MODULUS = NULL;
static PyObject *PyHASH_MODULUS_2 = NULL;
static PyObject *PyHASH_INF = NULL;

/*============================================================================
* Rational type
* ==========================================================================*/

// Variants of internal representation
#define RN_FPDEC 'D'        // value = sign * fpdec * 10 ^ exp
#define RN_U64_QUOT 'Q'     // value = sign * num / den
#define RN_PYINT_QUOT 'P'   // value = numerator / denominator

typedef struct rational_object {
    PyObject_HEAD
    Py_hash_t hash;
    struct {
        PyObject *numerator;
        PyObject *denominator;
    };
    char variant;               // see above
    rn_sign_t sign;             // 0 -> zero, -1 -> negative, 1 -> positive
    rn_prec_t prec;             // RN_UNLIM_PREC -> precision not constraint
    rn_exp_t exp;               // exponent of internal representation
    union {
        struct {
            uint64_t u64_num;
            uint64_t u64_den;
        };
        uint128_t coeff;
        };
    } RationalObject;

static PyTypeObject *RationalType;

#define RN_SIZE sizeof(RationalObject)
#define RN_VAR_OFFSET offsetof(RationalObject, variant)

#define RN_PYINT_QUOT_PTR(rn) \
    ((PyIntQuot *)&((RationalObject *)rn)->numerator)

// Method and helper prototypes

static PyObject *
Rational_as_fraction(RationalObject *self, PyObject *args UNUSED);

static error_t
rn_assert_num_den(RationalObject *rn);

// Type checks

static inline int
Rational_Check_Exact(PyObject *obj) {
    return (Py_TYPE(obj) == RationalType);
}

static inline int
Rational_Check(PyObject *obj) {
    return PyObject_TypeCheck(obj, RationalType);
}

// Raw data copy

static inline void
Rational_raw_data_copy(RationalObject *trgt, RationalObject *src) {
    memcpy((void *)trgt + RN_VAR_OFFSET,
           (void *)src + RN_VAR_OFFSET,
           RN_SIZE - RN_VAR_OFFSET);
}

// Constructors / destructors

static RationalObject *
RationalType_alloc(PyTypeObject *type) {
    RationalObject *rn;

    if (type == RationalType)
        rn = PyObject_New(RationalObject, type);
    else {
        allocfunc tp_alloc = (allocfunc)PyType_GetSlot(type, Py_tp_alloc);
        rn = (RationalObject *)tp_alloc(type, 0);
    }
    if (rn == NULL) {
        return NULL;
    }

    rn->hash = -1;
    rn->numerator = NULL;
    rn->denominator = NULL;
    rn->variant = RN_FPDEC;
    rn->sign = RN_SIGN_ZERO;
    rn->prec = 0;
    rn->exp = 0;
    rn->coeff = UINT128_ZERO;
    return rn;
}

static void
Rational_dealloc(RationalObject *self) {
    freefunc tp_free = (freefunc)PyType_GetSlot(Py_TYPE(self), Py_tp_free);
    Py_CLEAR(self->numerator);
    Py_CLEAR(self->denominator);
    tp_free(self);
}

#define RATIONAL_ALLOC(type, name) \
    RationalObject *name = RationalType_alloc(type); \
    do {if (name == NULL) return NULL; } while (0)

#define RATIONAL_ALLOC_SELF(type) \
    RATIONAL_ALLOC(type, self)

static inline void
rn_optimize_pyquot(RationalObject *rn) {
    assert(rn->variant == RN_PYINT_QUOT);
    int64_t num = PyLong_AsLongLong(rn->numerator);
    if (PyErr_Occurred()) {
        PyErr_Clear();
        return;
    }
    int64_t den = PyLong_AsLongLong(rn->denominator);
    if (PyErr_Occurred()) {
        PyErr_Clear();
        return;
    }
    if (num < 0)
        num = -num;
    if (rnd_from_quot(&rn->coeff, &rn->exp, num, den) == 0)
        rn->variant = RN_FPDEC;
    else {
        rn->variant = RN_U64_QUOT;
        rn->u64_num = num;
        rn->u64_den = den;
    }
}

static PyObject *
RationalType_from_rational_obj(PyTypeObject *type, RationalObject *rn) {
    if (type == RationalType) {
        // rn is a direct instance of RationalType and a direct instance of
        // RationalType is wanted, so just return the given instance
        // (ref count increased)
        Py_INCREF(rn);
        return (PyObject *)rn;
    }
    else {
        RATIONAL_ALLOC_SELF(type);
        Rational_raw_data_copy(self, rn);
        if (rn->numerator != NULL) {
            Py_INCREF(rn->numerator);
            self->numerator = rn->numerator;
            Py_INCREF(rn->denominator);
            self->denominator = rn->denominator;
        }
        self->hash = rn->hash;
        return (PyObject *)self;
    }
}

static PyObject *
RationalType_from_pylong(PyTypeObject *type, PyObject *val) {
    long long lval;
    PyObject *abs_val = NULL;
    PyObject *lo = NULL;
    PyObject *hi = NULL;

    RATIONAL_ALLOC_SELF(type);
    if (PyObject_RichCompareBool(val, PyZERO, Py_EQ))
        return (PyObject *)self;
    Py_INCREF(val);
    self->numerator = val;
    Py_INCREF(PyONE);
    self->denominator = PyONE;
    // try optimized variant
    // val fits into long long?
    lval = PyLong_AsLongLong(val);
    if (!PyErr_Occurred()) {
        if (lval > 0) {
            self->sign = RN_SIGN_POS;
            U128_FROM_LO_HI(&self->coeff, lval, 0ULL);
        }
        else  {
            self->sign = RN_SIGN_NEG;
            U128_FROM_LO_HI(&self->coeff, -lval, 0ULL);
        }
        goto CLEAN_UP;
    }
    PyErr_Clear();  // fall through
    // get sign and absolute value
    if (PyObject_RichCompareBool(val, PyZERO, Py_GE)) {
        self->sign = RN_SIGN_POS;
        Py_INCREF(val);
        abs_val = val;
    }
    else {
        self->sign = RN_SIGN_NEG;
        ASSIGN_AND_CHECK_NULL(abs_val, PyNumber_Absolute(val));
    }
    // abs_val fits into uint128?
    ASSIGN_AND_CHECK_NULL(hi, PyNumber_Rshift(abs_val, Py64));
    ASSIGN_AND_CHECK_NULL(lo, PyNumber_And(abs_val, PyUInt64Max));
    U128_FROM_LO_HI(&self->coeff,
                    PyLong_AsUnsignedLongLong(lo),
                    PyLong_AsUnsignedLongLong(hi));
    if (!PyErr_Occurred())
        goto CLEAN_UP;

ERROR:
    PyErr_Clear();
    self->variant = RN_PYINT_QUOT;

CLEAN_UP:
    Py_XDECREF(abs_val);
    Py_XDECREF(hi);
    Py_XDECREF(lo);
    return (PyObject *)self;
}

static PyObject *
RationalType_from_integral(PyTypeObject *type, PyObject *val) {
    PyObject *rn;
    PyObject *i = PyNumber_Long(val);
    if (i == NULL)
        return NULL;
    rn = RationalType_from_pylong(type, i);
    Py_DECREF(i);
    return rn;
}

static PyObject *
RationalType_from_normalized_num_den(PyTypeObject *type, PyObject *numerator,
                                     PyObject *denominator) {
    RATIONAL_ALLOC_SELF(type);

    if (PyObject_RichCompareBool(numerator, PyZERO, Py_EQ))
        return (PyObject *)self;

    if (PyObject_RichCompareBool(numerator, PyZERO, Py_LT))
        self->sign = RN_SIGN_NEG;
    else
        self->sign = RN_SIGN_POS;
    self->variant = RN_PYINT_QUOT;
    self->exp = RN_UNDEF_EXP;
    Py_INCREF(numerator);
    self->numerator = numerator;
    Py_INCREF(denominator);
    self->denominator = denominator;
    rn_optimize_pyquot(self);
    return (PyObject *)self;
}

static PyObject *
RationalType_from_num_den(PyTypeObject *type, PyObject *numerator,
                          PyObject *denominator) {
    PyObject *ratio = NULL;
    PyObject *t = NULL;
    PyIntQuot quot = {NULL, NULL};
    PyObject *res = NULL;

    if (PyLong_Check(numerator)) {
        Py_INCREF(numerator);
        quot.numerator = numerator;
    }
    else {
        ASSIGN_AND_CHECK_NULL(ratio,
                              PyObject_CallMethod(numerator,
                                                  "as_integer_ratio", NULL));
        ASSIGN_AND_CHECK_NULL(quot.numerator,
                              PySequence_GetItem(ratio, 0));
        ASSIGN_AND_CHECK_NULL(quot.denominator,
                              PySequence_GetItem(ratio, 1));
        Py_CLEAR(ratio);
    }
    if (PyLong_Check(denominator)) {
        if (quot.denominator == NULL) {
            Py_INCREF(denominator);
            quot.denominator = denominator;
        }
        else
            ASSIGN_AND_CHECK_NULL(quot.denominator,
                                  PyNumber_InPlaceMultiply(quot.denominator,
                                                           denominator));
    }
    else {
        ASSIGN_AND_CHECK_NULL(ratio,
                              PyObject_CallMethod(denominator,
                                                  "as_integer_ratio", NULL));
        ASSIGN_AND_CHECK_NULL(t, PySequence_GetItem(ratio, 1));
        ASSIGN_AND_CHECK_NULL(quot.numerator,
                              PyNumber_InPlaceMultiply(quot.numerator, t));
        Py_CLEAR(t);
        if (quot.denominator == NULL)
            ASSIGN_AND_CHECK_NULL(quot.denominator,
                                  PySequence_GetItem(ratio, 0));
        else {
            ASSIGN_AND_CHECK_NULL(t, PySequence_GetItem(ratio, 0));
            ASSIGN_AND_CHECK_NULL(quot.denominator,
                                  PyNumber_InPlaceMultiply(quot.denominator,
                                                           t));
            Py_CLEAR(t);
        }
        Py_CLEAR(ratio);
    }
    if (PyObject_RichCompareBool(quot.denominator, PyZERO, Py_LT)) {
        t = quot.numerator;
        ASSIGN_AND_CHECK_NULL(quot.numerator, PyNumber_Negative(t));
        Py_CLEAR(t);
        t = quot.denominator;
        ASSIGN_AND_CHECK_NULL(quot.denominator, PyNumber_Negative(t));
        Py_CLEAR(t);
    }
    if (rnp_reduce_inplace(&quot) < 0)
        goto ERROR;
    res = RationalType_from_normalized_num_den(type, quot.numerator,
                                               quot.denominator);
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(ratio);
    Py_XDECREF(t);
    Py_XDECREF(quot.numerator);
    Py_XDECREF(quot.denominator);
    return res;
}

static PyObject *
RationalType_from_rational(PyTypeObject *type, PyObject *val) {
    PyObject *numerator = NULL;
    PyObject *denominator = NULL;
    PyObject *res = NULL;

    ASSIGN_AND_CHECK_NULL(numerator,
                          PyObject_GetAttrString(val, "numerator"));
    ASSIGN_AND_CHECK_NULL(denominator,
                          PyObject_GetAttrString(val, "denominator"));
    ASSIGN_AND_CHECK_NULL(res,
                          RationalType_from_normalized_num_den(type, numerator,
                                                               denominator));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(numerator);
    Py_XDECREF(denominator);
    return res;
}

static PyObject *
RationalType_from_str(PyTypeObject *type, PyObject *val) {
    Py_UCS4 *buf;
    error_t rc;
    struct rn_parsed_repr parsed;
    PyObject *frac = NULL;
    PyObject *res = NULL;

    ASSIGN_AND_CHECK_NULL(buf, PyUnicode_AsUCS4Copy(val));
    rc = rn_from_ucs4_literal(&parsed, buf);
    PyMem_Free(buf);
    if (rc < 0) {
        if (PyErr_Occurred())
            goto ERROR;
        goto FALLBACK;  // fall back to Fraction
    }
    else {
        RATIONAL_ALLOC_SELF(type);
        res = (PyObject *)self;
        if (parsed.is_quot) {
            if (parsed.den == 0) {
                PyErr_SetString(PyExc_ZeroDivisionError, "Denominator = 0.");
                goto ERROR;
            }
            if (parsed.num == 0)
                goto CLEAN_UP;
            rnq_reduce_quot(&parsed.num, &parsed.den);
            if (rnd_from_quot(&self->coeff, &self->exp, parsed.num,
                              parsed.den) == 0) {
                self->variant = RN_FPDEC;
                self->prec = -self->exp;
            }
            else {
                self->variant = RN_U64_QUOT;
                self->u64_num = parsed.num;
                self->u64_den = parsed.den;
                self->exp = RN_UNDEF_EXP;
                self->prec = RN_UNLIM_PREC;
            }
        }
        else if (U128_EQ_ZERO(parsed.coeff))
            goto CLEAN_UP;
        else {
            self->variant = RN_FPDEC;
            self->coeff = parsed.coeff;
            self->exp = parsed.exp;
            self->prec = -parsed.exp;
        }
        if (parsed.neg)
            self->sign = RN_SIGN_NEG;
        else
            self->sign = RN_SIGN_POS;
        goto CLEAN_UP;
    }

FALLBACK:
    ASSIGN_AND_CHECK_NULL(frac,
                          PyObject_CallFunctionObjArgs(Fraction, val,
                                                       NULL));
    ASSIGN_AND_CHECK_NULL(res, RationalType_from_rational(type, frac));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());
    Py_CLEAR(res);

CLEAN_UP:
    Py_XDECREF(frac);
    return res;
}

static PyObject *
RationalType_from_float(PyTypeObject *type, PyObject *val) {
    PyObject *res = NULL;
    PyObject *ratio = NULL;
    PyObject *numerator = NULL;
    PyObject *denominator = NULL;

    ASSIGN_AND_CHECK_NULL(ratio,
                          PyObject_CallMethod(val, "as_integer_ratio", NULL));
    ASSIGN_AND_CHECK_NULL(numerator, PySequence_GetItem(ratio, 0));
    ASSIGN_AND_CHECK_NULL(denominator, PySequence_GetItem(ratio, 1));
    ASSIGN_AND_CHECK_NULL(res,
                          RationalType_from_normalized_num_den(type, numerator,
                                                               denominator));
    goto CLEAN_UP;

ERROR:
    {
        PyObject *err = PyErr_Occurred();
        assert(err);
        if (err == PyExc_ValueError || err == PyExc_OverflowError ||
            err == PyExc_AttributeError) {
            PyErr_Clear();
            PyErr_Format(PyExc_ValueError, "Can't convert %R to Rational.",
                         val);
        }
    }

CLEAN_UP:
    Py_XDECREF(ratio);
    Py_XDECREF(numerator);
    Py_XDECREF(denominator);
    return res;
}

static PyObject *
RationalType_from_decimal(PyTypeObject *type, PyObject *val) {
    PyObject *res = NULL;
    PyObject *is_finite = NULL;

    ASSIGN_AND_CHECK_NULL(is_finite,
                          PyObject_CallMethod(val, "is_finite", NULL));
    if (!PyObject_IsTrue(is_finite)) {
        PyErr_Format(PyExc_ValueError, "Can't convert %R to Rational.", val);
        goto ERROR;
    }
    ASSIGN_AND_CHECK_NULL(res, RationalType_from_float(type, val));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(is_finite);
    return res;
}

static PyObject *
RationalType_from_float_or_int(PyTypeObject *type, PyObject *val) {
    if (PyFloat_Check(val))
        return RationalType_from_float(type, val);
    if (PyLong_Check(val)) // NOLINT(hicpp-signed-bitwise)
        return RationalType_from_pylong(type, val);
    return PyErr_Format(PyExc_TypeError, "%R is not a float or int.", val);
}

static PyObject *
RationalType_from_decimal_or_int(PyTypeObject *type, PyObject *val) {
    if (PyObject_IsInstance(val, Decimal))
        return RationalType_from_decimal(type, val);
    if (PyLong_Check(val)) // NOLINT(hicpp-signed-bitwise)
        return RationalType_from_pylong(type, val);
    if (PyObject_IsInstance(val, Integral))
        return RationalType_from_integral(type, val);
    return PyErr_Format(PyExc_TypeError, "%R is not a Decimal or Integral.",
                        val);
}

static PyObject *
RationalType_from_obj(PyTypeObject *type, PyObject *obj) {

    if (obj == Py_None) {
        RATIONAL_ALLOC_SELF(type);
        Py_INCREF(PyZERO);
        self->numerator = PyZERO;
        Py_INCREF(PyONE);
        self->denominator = PyONE;
        return (PyObject *)self;
    }

    // Rational
    if (Rational_Check(obj))
        return RationalType_from_rational_obj(type, (RationalObject *)obj);

    // String
    if (PyUnicode_Check(obj)) // NOLINT(hicpp-signed-bitwise)
        return RationalType_from_str(type, obj);

    // Python <int>
    if (PyLong_Check(obj)) // NOLINT(hicpp-signed-bitwise)
        return RationalType_from_pylong(type, obj);

    // Integral
    if (PyObject_IsInstance(obj, Integral))
        return RationalType_from_integral(type, obj);

    // abstract Rational
    if (PyObject_IsInstance(obj, Rational))
        return RationalType_from_rational(type, obj);

    // Decimal
    if (PyObject_IsInstance(obj, Decimal))
        return RationalType_from_decimal(type, obj);

    // Python <float>, Real
    if (PyFloat_Check(obj) || PyObject_IsInstance(obj, Real))
        return RationalType_from_float(type, obj);

    // unable to create Rational
    return PyErr_Format(PyExc_TypeError, "Can't convert %R to Rational.", obj);
}

static PyObject *
RationalType_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    static char *kw_names[] = {"numerator", "denominator", NULL};
    PyObject *numerator = Py_None;
    PyObject *denominator = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kw_names,
                                     &numerator, &denominator))
        return NULL;

    if (denominator == Py_None)
        return (PyObject *)RationalType_from_obj(type, numerator);
    if (PyObject_RichCompareBool(denominator, PyZERO, Py_EQ))
        return value_error_ptr("Denominator must not be zero.");
    return RationalType_from_num_den(type, numerator, denominator);
}

// Helper macros

#define RATIONAL_ALLOC_RESULT(type) \
    RATIONAL_ALLOC(type, res)

#define BINOP_RN_TYPE(x, y)              \
    PyTypeObject *rn_type =              \
        Rational_Check(x) ? Py_TYPE(x) : \
        (Rational_Check(y) ? Py_TYPE(y) : NULL)

// Properties

static PyObject *
Rational_precision_get(RationalObject *self, void *closure UNUSED) {
    rn_prec_t prec = self->prec;
    if (prec == RN_UNLIM_PREC)
        return Py_None;
    return PyLong_FromLong(prec);
}

static PyObject *
Rational_magnitude_get(RationalObject *self, void *closure UNUSED) {
    int magn;

    if (self->sign == 0) {
        PyErr_SetString(PyExc_OverflowError, "Result would be '-Infinity'.");
        return NULL;
    }
    switch (self->variant) {
        case RN_FPDEC:
            magn = rnd_magnitude(self->coeff, self->exp);
            break;
        case RN_U64_QUOT:
            magn = rnq_magnitude(self->u64_num, self->u64_den);
            break;
        case RN_PYINT_QUOT:
            return rnp_magnitude(RN_PYINT_QUOT_PTR(self));
        default:
            PyErr_SetString(PyExc_RuntimeError,
                            "Corrupted internal representation.");
            return NULL;
    }
    return PyLong_FromLong(magn);
}

static error_t
rn_assert_num_den(RationalObject *rn) {
    error_t rc = 0;
    PyObject *num = NULL;
    PyObject *exp = NULL;

    if (rn->numerator == NULL) {
        assert(rn->denominator == NULL);
        switch (rn->variant) {
            case RN_FPDEC:
                if (rn->sign == RN_SIGN_NEG) {
                    ASSIGN_AND_CHECK_NULL(num, pylong_from_u128(&rn->coeff));
                    ASSIGN_AND_CHECK_NULL(rn->numerator,
                                          PyNumber_Negative(num));
                }
                else
                    ASSIGN_AND_CHECK_NULL(rn->numerator,
                                          pylong_from_u128(&rn->coeff));
                if (rn->exp == 0) {
                    Py_INCREF(PyONE);
                    rn->denominator = PyONE;
                }
                else if (rn->exp < 0) {
                    uint64_t d;
                    rn_exp_t t = -rn->exp;
                    if (t < UINT64_10_POW_N_CUTOFF) {
                        d = u64_10_pow_n(t);
                        ASSIGN_AND_CHECK_NULL(rn->denominator,
                                              PyLong_FromLongLong(d));
                    }
                    else {
                        ASSIGN_AND_CHECK_NULL(exp, PyLong_FromLong(t));
                        ASSIGN_AND_CHECK_NULL(rn->denominator,
                                              PyNumber_Power(PyTEN, exp,
                                                             Py_None));
                    }
                }
                else {
                    Py_XDECREF(num);
                    if (rn->exp < UINT64_10_POW_N_CUTOFF) {
                        uint64_t t = u64_10_pow_n(rn->exp);
                        ASSIGN_AND_CHECK_NULL(num, PyLong_FromLong(t));
                        ASSIGN_AND_CHECK_NULL(rn->numerator,
                                              PyNumber_InPlaceMultiply
                                              (rn->numerator, num));
                    }
                    else {
                        ASSIGN_AND_CHECK_NULL(exp, PyLong_FromLong(rn->exp));
                        ASSIGN_AND_CHECK_NULL(num, PyNumber_Power(PyTEN, exp,
                                                                  Py_None));
                        ASSIGN_AND_CHECK_NULL(rn->numerator,
                                              PyNumber_InPlaceMultiply
                                              (rn->numerator, num));
                    }
                    Py_INCREF(PyONE);
                    rn->denominator = PyONE;
                }
                // TODO: check whether this is neccessary
                if (rnp_reduce_inplace(RN_PYINT_QUOT_PTR(rn)) < 0)
                    goto ERROR;
                break;
            case RN_U64_QUOT:
                if (rn->sign == RN_SIGN_NEG) {
                    ASSIGN_AND_CHECK_NULL(num,
                                          PyLong_FromLongLong(rn->u64_num));
                    ASSIGN_AND_CHECK_NULL(rn->numerator,
                                          PyNumber_Negative(num));
                }
                else
                    ASSIGN_AND_CHECK_NULL(rn->numerator,
                                          PyLong_FromLongLong(rn->u64_num));
                ASSIGN_AND_CHECK_NULL(rn->denominator,
                                      PyLong_FromLongLong(rn->u64_den));
                break;
            default:
                PyErr_SetString(PyExc_RuntimeError,
                                "Corrupted internal representation.");
                goto ERROR;
        }
    }
    assert(rn->numerator != NULL && rn->denominator != NULL);
    goto CLEAN_UP;

ERROR:
    Py_CLEAR(rn->numerator);
    Py_CLEAR(rn->denominator);
    rc = -1;

CLEAN_UP:
    Py_XDECREF(num);
    Py_XDECREF(exp);
    return rc;
}

static PyObject *
Rational_numerator_get(RationalObject *self, void *closure UNUSED) {
    if (rn_assert_num_den(self) != 0)
            return NULL;
    Py_INCREF(self->numerator);
    return self->numerator;
}

static PyObject *
Rational_denominator_get(RationalObject *self, void *closure UNUSED) {
    if (rn_assert_num_den(self) != 0)
        return NULL;
    Py_INCREF(self->denominator);
    return self->denominator;
}

static PyObject *
Rational_real_get(RationalObject *self, void *closure UNUSED) {
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
Rational_imag_get(RationalObject *self UNUSED, void *closure UNUSED) {
    Py_INCREF(PyZERO);
    return PyZERO;
}

// String representation

static PyObject *
Rational_bytes(RationalObject *self, PyObject *args UNUSED) {
    PyObject *res = NULL;
/*
    char *lit = fpdec_as_ascii_literal(&self->coeff, false);

    if (lit == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    res = PyBytes_FromString(lit);
    fpdec_mem_free(lit);
*/
    return res;
}

static PyObject *
Rational_str(RationalObject *self) {
    PyObject *res = NULL;
    rn_sign_t sign = self->sign;
    rn_exp_t exp;
    uint128_t coeff;
    uint64_t sh, frac;

    if (sign == 0)
        return PyUnicode_FromString("0");

    switch (self->variant) {
        case RN_FPDEC:
            exp = self->exp;
            coeff = self->coeff;
            if (exp < 0) {
                sh = u64_10_pow_n(-exp);
                frac = u128_idiv_u64(&coeff, sh);
            }
            else
                frac = 0;
            if (U128_HI(coeff) == 0) {
                if (frac > 0)
                    res = PyUnicode_FromFormat("%llu.%llu",
                                               U128_LO(coeff), frac);
                else
                    res = PyUnicode_FromFormat("%llu", U128_LO(coeff));
            }
            else
                return PyUnicode_FromString("Not yet implemented");
            break;
        case RN_U64_QUOT:
            if (self->u64_den == 1)
                res = PyUnicode_FromFormat("%llu", self->u64_num);
            else
                res = PyUnicode_FromFormat("%llu/%llu", self->u64_num,
                                           self->u64_den);
            break;
        case RN_PYINT_QUOT:
            if (PyObject_RichCompareBool(self->denominator, PyONE, Py_EQ))
                res = PyUnicode_FromFormat("%S", self->numerator);
            else
                res = PyUnicode_FromFormat("%S/%S", self->numerator,
                                           self->denominator);
            break;
    }
    return res;
}

static PyObject *
Rational_repr(RationalObject *self) {
    PyObject *res = NULL;
    PyObject *cls_name = NULL;
    cls_name = PyObject_GetAttrString((PyObject *)Py_TYPE(self), "__name__");
    if (cls_name == NULL)
        return NULL;
    res = PyUnicode_FromFormat("%S('%S')", cls_name, self);
    Py_DECREF(cls_name);
    return res;
}

static PyObject *
Rational_format(RationalObject *self, PyObject *fmt_spec) {
    PyObject *res = NULL;
/*
    PyObject *utf8_fmt_spec = NULL;
    uint8_t *formatted = NULL;

    ASSIGN_AND_CHECK_NULL(utf8_fmt_spec, PyUnicode_AsUTF8String(fmt_spec));
    formatted = fpdec_formatted(&self->coeff,
                                (uint8_t *)PyBytes_AsString(utf8_fmt_spec));
    if (formatted == NULL)
        CHECK_FPDEC_ERROR(errno);
    ASSIGN_AND_CHECK_NULL(res, PyUnicode_FromString((char *)formatted));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(utf8_fmt_spec);
    fpdec_mem_free(formatted);
*/
    return res;
}


// Special methods

static Py_hash_t
Rational_hash(RationalObject *self) {
    Py_hash_t res = self->hash;

    if (res != -1)
        return res;

    PyObject *inv_den = NULL;
    PyObject *abs_num = NULL;
    PyObject *t = NULL;
    /* To make sure that the hash of a Rational equals the hash of a
     * numerically equal integer, float or Fraction instance, we follow the
     * implementation in fractions.py.
     * For the general rules for numeric hashes see the library docs,
     * 'Built-in Types' / 'Hashing of numeric types'. */
    ASSIGN_AND_CHECK_NULL(inv_den, PyNumber_Power(self->denominator,
                                                  PyHASH_MODULUS_2,
                                                  PyHASH_MODULUS));
    if (PyObject_RichCompareBool(inv_den, PyZERO, Py_EQ))
        res = PyHASH_INF;
    else {
        // Optimized implementation from Python 3.9
        ASSIGN_AND_CHECK_NULL(abs_num, PyNumber_Absolute(self->numerator));
        ASSIGN_AND_CHECK_NULL(t,
                              PyLong_FromLongLong(PyObject_Hash(abs_num)));
        ASSIGN_AND_CHECK_NULL(t, PyNumber_InPlaceMultiply(t, inv_den));
        res = PyObject_Hash(t);
    }
    if (self->sign < 0)
        res = -res;
    self->hash = res;
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(inv_den);
    Py_XDECREF(abs_num);
    Py_XDECREF(t);
    return res;
}

static PyObject *
Rational_copy(RationalObject *self, PyObject *args UNUSED) {
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
Rational_deepcopy(RationalObject *self, PyObject *memo UNUSED) {
    Py_INCREF(self);
    return (PyObject *)self;
}

static inline int
Rational_cmp(RationalObject *self, RationalObject *other) {
    int8_t cmp = CMP(self->sign, other->sign);
    if (cmp != 0 || self->sign == 0)
        return cmp;
    if (self->variant == RN_PYINT_QUOT)
        return rnp_cmp(RN_PYINT_QUOT_PTR(self), RN_PYINT_QUOT_PTR(other));
    PyErr_SetString(PyExc_RuntimeError, "Corrupted internal representation.");
    return 0;
}

static PyObject *
Rational_cmp_to_int(RationalObject *self, PyObject *other, int op) {
    PyObject *res = NULL;
    PyObject *num = NULL;
    PyObject *den = NULL;
    PyObject *t = NULL;

    ASSIGN_AND_CHECK_NULL(num, Rational_numerator_get(self, NULL));
    ASSIGN_AND_CHECK_NULL(den, Rational_denominator_get(self, NULL));
    ASSIGN_AND_CHECK_NULL(t, PyNumber_Multiply(other, den));
    ASSIGN_AND_CHECK_NULL(res, PyObject_RichCompare(num, t, op));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());
    Py_CLEAR(res);

CLEAN_UP:
    Py_XDECREF(num);
    Py_XDECREF(den);
    Py_XDECREF(t);
    return res;
}

static PyObject *
Rational_cmp_to_ratio(RationalObject *self, PyObject *other, int op) {
    PyObject *res = NULL;
    PyObject *x_num = NULL;
    PyObject *x_den = NULL;
    PyObject *y_num = NULL;
    PyObject *y_den = NULL;
    PyObject *lhs = NULL;
    PyObject *rhs = NULL;

    ASSIGN_AND_CHECK_NULL(x_num, Rational_numerator_get(self, NULL));
    ASSIGN_AND_CHECK_NULL(x_den, Rational_denominator_get(self, NULL));
    ASSIGN_AND_CHECK_NULL(y_num, PySequence_GetItem(other, 0));
    ASSIGN_AND_CHECK_NULL(y_den, PySequence_GetItem(other, 1));
    ASSIGN_AND_CHECK_NULL(lhs, PyNumber_Multiply(x_num, y_den));
    ASSIGN_AND_CHECK_NULL(rhs, PyNumber_Multiply(y_num, x_den));
    ASSIGN_AND_CHECK_NULL(res, PyObject_RichCompare(lhs, rhs, op));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());
    Py_CLEAR(res);

CLEAN_UP:
    Py_XDECREF(x_num);
    Py_XDECREF(x_den);
    Py_XDECREF(y_num);
    Py_XDECREF(y_den);
    Py_XDECREF(lhs);
    Py_XDECREF(rhs);
    return res;
}

static PyObject *
Rational_richcompare(RationalObject *self, PyObject *other, int op) {
    PyObject *res = NULL;
    PyObject *t = NULL;

    // Rational
    if (Rational_Check(other)) {
        int r = Rational_cmp(self, (RationalObject *)other);
        Py_RETURN_RICHCOMPARE(r, 0, op);
    }

    // Python <int>
    if (PyLong_Check(other)) // NOLINT(hicpp-signed-bitwise)
        return Rational_cmp_to_int(self, other, op);

    // Integral
    if (PyObject_IsInstance(other, Integral)) {
        ASSIGN_AND_CHECK_NULL(t, PyNumber_Long(other));
        ASSIGN_AND_CHECK_NULL(res, Rational_cmp_to_int(self, t, op));
        goto CLEAN_UP;
    }

    // Rational ABC
    if (PyObject_IsInstance(other, Rational)) {
        ASSIGN_AND_CHECK_NULL(t, Rational_as_fraction(self, NULL));
        ASSIGN_AND_CHECK_NULL(res, PyObject_RichCompare(t, other, op));
        goto CLEAN_UP;
    }

    // Python <float>, Decimal, Real
    // Test if convertable to a Rational
    t = PyObject_CallMethod(other, "as_integer_ratio", NULL);
    if (t != NULL) {
        res = Rational_cmp_to_ratio(self, t, op);
        goto CLEAN_UP;
    }
    else {
        PyObject *exc = PyErr_Occurred();
        if (exc == PyExc_ValueError || exc == PyExc_OverflowError) {
            // 'nan' or 'inf'
            PyErr_Clear();
            res = PyObject_RichCompare(PyZERO, other, op);
            goto CLEAN_UP;
        }
        else if (exc == PyExc_AttributeError)
            // fall through
            PyErr_Clear();
        else
            goto ERROR;
    }

    // Complex
    if (PyObject_IsInstance(other, Complex)) {
        if (op == Py_EQ || op == Py_NE) {
            ASSIGN_AND_CHECK_NULL(t, PyObject_GetAttrString(other, "imag"));
            if (PyObject_RichCompareBool(t, PyZERO, Py_EQ)) {
                Py_DECREF(t);
                ASSIGN_AND_CHECK_NULL(t, PyObject_GetAttrString(other,
                                                                "real"));
                ASSIGN_AND_CHECK_NULL(res, Rational_richcompare(self, t, op));
            }
            else {
                res = op == Py_EQ ? Py_False : Py_True;
                Py_INCREF(res);
            }
            goto CLEAN_UP;
        }
    }

    // don't know how to compare
    Py_INCREF(Py_NotImplemented);
    res = Py_NotImplemented;
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());
    Py_CLEAR(res);

CLEAN_UP:
    Py_CLEAR(t);
    return res;
}

// Unary number methods

static PyObject *
Rational_neg(RationalObject *self) {
    if (self->sign == RN_SIGN_ZERO) {
        Py_INCREF(self);
        return (PyObject *)self;
    }
    RATIONAL_ALLOC(Py_TYPE(self), res);
    Rational_raw_data_copy(res, self);
    res->sign *= -1;
    if (self->numerator != NULL) {
        res->numerator = PyNumber_Negative(self->numerator);
        Py_INCREF(self->denominator);
        res->denominator = self->denominator;
    }
    return (PyObject *)res;
}

static PyObject *
Rational_pos(PyObject *self) {
    Py_INCREF(self);
    return self;
}

static PyObject *
Rational_abs(RationalObject *self) {
    if (self->sign != RN_SIGN_NEG) {
        Py_INCREF(self);
        return (PyObject *)self;
    }
    RATIONAL_ALLOC(Py_TYPE(self), res);
    Rational_raw_data_copy(res, self);
    res->sign = RN_SIGN_POS;
    if (self->numerator != NULL) {
        res->numerator = PyNumber_Absolute(self->numerator);
        Py_INCREF(self->denominator);
        res->denominator = self->denominator;
    }
    return (PyObject *)res;
}

static PyObject *
Rational_int(RationalObject *self, PyObject *args UNUSED) {
    if (self->variant == RN_PYINT_QUOT)
        return rnp_to_int(RN_PYINT_QUOT_PTR(self));
    PyErr_SetString(PyExc_RuntimeError, "Corrupted internal representation.");
    return NULL;
}

static PyObject *
Rational_float(RationalObject *self, PyObject *args UNUSED) {
    if (self->variant == RN_PYINT_QUOT)
        return rnp_to_float(RN_PYINT_QUOT_PTR(self));
    PyErr_SetString(PyExc_RuntimeError, "Corrupted internal representation.");
    return NULL;
}

static int
Rational_bool(RationalObject *self) {
    return self->sign != RN_SIGN_ZERO;
}

// Binary number methods

/*
static fpdec_t *
fpdec_from_number(fpdec_t *tmp, PyObject *obj) {
    PyObject *ratio = NULL;
    PyObject *num = NULL;
    PyObject *den = NULL;
    error_t rc;
    fpdec_t *fpdec = NULL;

    if (Rational_Check(obj)) {
        fpdec = &((RationalObject *)obj)->coeff;
    }
    else if (PyLong_Check(obj)) { // NOLINT(hicpp-signed-bitwise)
        rc = fpdec_from_pylong(tmp, obj);
        if (rc == FPDEC_OK)
            fpdec = tmp;
    }
    else if (PyObject_IsInstance(obj, Integral)) {
        ASSIGN_AND_CHECK_NULL(num, PyNumber_Long(obj));
        rc = fpdec_from_pylong(tmp, num);
        if (rc == FPDEC_OK)
            fpdec = tmp;
    }
    else if (PyObject_IsInstance(obj, Rational)) {
        ASSIGN_AND_CHECK_NULL(num, PyObject_GetAttrString(obj, "numerator"));
        ASSIGN_AND_CHECK_NULL(den,
                              PyObject_GetAttrString(obj, "denominator"));
        rc = fpdec_from_num_den(tmp, num, den, -1);
        if (rc == FPDEC_OK)
            fpdec = tmp;
    }
    else if (PyObject_IsInstance(obj, Real) ||
             PyObject_IsInstance(obj, Decimal)) {
        ASSIGN_AND_CHECK_NULL(ratio,
                              PyObject_CallMethod(obj, "as_integer_ratio",
                                                  NULL));
        ASSIGN_AND_CHECK_NULL(num, PySequence_GetItem(ratio, 0));
        ASSIGN_AND_CHECK_NULL(den, PySequence_GetItem(ratio, 1));
        rc = fpdec_from_num_den(tmp, num, den, -1);
        if (rc == FPDEC_OK)
            fpdec = tmp;
    }
    goto CLEAN_UP;

ERROR:
    {
        PyObject *err = PyErr_Occurred();
        assert(err);
        if (err == PyExc_ValueError || err == PyExc_OverflowError ||
            err == PyExc_AttributeError) {
            PyErr_Clear();
            PyErr_Format(PyExc_ValueError, "Unsupported operand: %R.", obj);
        }
    }

CLEAN_UP:
    Py_XDECREF(ratio);
    Py_XDECREF(num);
    Py_XDECREF(den);
    return fpdec;
}

static PyObject *
fallback_op(PyObject *x, PyObject *y, binaryfunc op) {
    PyObject *res = NULL;
    PyObject *fx = NULL;
    PyObject *fy = NULL;

    ASSIGN_AND_CHECK_NULL(fx, PyObject_CallFunctionObjArgs(Fraction, x,
                                                           Py_None, NULL));
    ASSIGN_AND_CHECK_NULL(fy, PyObject_CallFunctionObjArgs(Fraction, y,
                                                           Py_None, NULL));
    ASSIGN_AND_CHECK_NULL(res, op(fx, fy));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(fx);
    Py_XDECREF(fy);
    return res;
}

static PyObject *
fallback_n_convert_op(PyTypeObject *dec_type, PyObject *x, PyObject *y,
                      binaryfunc op) {
    PyObject *res = NULL;
    PyObject *dec = NULL;

    ASSIGN_AND_CHECK_NULL(res, fallback_op(x, y, op));
    // try to convert result back to Rational
    dec = RationalType_from_rational(dec_type, res, -1);
    if (dec == NULL) {
        // result is not convertable to a Rational, so return Fraction
        PyErr_Clear();
    }
    else {
        Py_CLEAR(res);
        res = dec;
    }
    return res;

ERROR:
    assert(PyErr_Occurred());
    return NULL;
}
*/

static inline PyObject *
rn_add(RationalObject *x, PyObject *y) {
    PyIntQuot qy = {NULL, NULL};
    PyIntQuot *pqy = NULL;

    if (Rational_Check(y)) {
        CHECK_RC(rn_assert_num_den((RationalObject *)y));
        pqy = RN_PYINT_QUOT_PTR(y);
    }
    else if (rnp_from_number(&qy, y) == 0)
        pqy = &qy;
    else
        // can't convert y to integer ratio, so give up
        Py_RETURN_NOTIMPLEMENTED;

    RATIONAL_ALLOC_RESULT(Py_TYPE(x));
    if (x->variant == RN_PYINT_QUOT) {
        CHECK_RC(rnp_add(RN_PYINT_QUOT_PTR(res), RN_PYINT_QUOT_PTR(x), pqy));
        res->variant = RN_PYINT_QUOT;
        goto CLEAN_UP;
    }

ERROR:
    assert(PyErr_Occurred());
    Py_CLEAR(res);

CLEAN_UP:
    return (PyObject *)res;
}

static PyObject *
Rational_add(PyObject *x, PyObject *y) {
    if (Rational_Check(x))
        return rn_add((RationalObject *)x, y);
    else
        return rn_add((RationalObject *)y, x);
}

static PyObject *
Rational_sub(PyObject *x, PyObject *y) {
    RATIONAL_ALLOC_RESULT(Py_TYPE(x));
    return (PyObject *)res;
}

static PyObject *
Rational_mul(PyObject *x, PyObject *y) {
    RATIONAL_ALLOC_RESULT(Py_TYPE(x));
    return (PyObject *)res;
}

static PyObject *
Rational_remainder(PyObject *x, PyObject *y) {
    RATIONAL_ALLOC_RESULT(Py_TYPE(x));
    return (PyObject *)res;
}

static PyObject *
Rational_divmod(PyObject *x, PyObject *y) {
    RATIONAL_ALLOC_RESULT(Py_TYPE(x));
    return (PyObject *)res;
}

static PyObject *
Rational_floordiv(PyObject *x, PyObject *y) {
    RATIONAL_ALLOC_RESULT(Py_TYPE(x));
    return (PyObject *)res;
}

static PyObject *
Rational_truediv(PyObject *x, PyObject *y) {
    RATIONAL_ALLOC_RESULT(Py_TYPE(x));
    return (PyObject *)res;
}

// Ternary number methods

static PyObject *
rn_pow_pylong(RationalObject *x, PyObject *exp) {
    RATIONAL_ALLOC_RESULT(Py_TYPE(x));
    return (PyObject *)res;
}

static PyObject *
rn_pow_obj(RationalObject *x, PyObject *y) {
    PyObject *res = NULL;
    PyObject *exp = NULL;
    PyObject *fx = NULL;
    PyObject *fy = NULL;

    exp = PyNumber_Long(y);
    if (exp == NULL) {
        PyObject *exc = PyErr_Occurred();
        if (exc == PyExc_ValueError || exc == PyExc_OverflowError) {
            PyErr_Clear();
            PyErr_Format(PyExc_ValueError, "Unsupported operand: %R", y);
        }
        goto ERROR;
    }
    if (PyObject_RichCompareBool(exp, y, Py_NE) == 1) {
        // fractional exponent => fallback to float
        ASSIGN_AND_CHECK_NULL(fx, PyNumber_Float((PyObject *)x));
        ASSIGN_AND_CHECK_NULL(fy, PyNumber_Float(y));
        ASSIGN_AND_CHECK_NULL(res, PyNumber_Power(fx, fy, Py_None));
    }
    else
        ASSIGN_AND_CHECK_NULL(res, rn_pow_pylong(x, exp));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(exp);
    Py_XDECREF(fx);
    Py_XDECREF(fy);
    return res;
}

static PyObject *
obj_pow_rn(PyObject *x, RationalObject *y) {
    PyObject *res = NULL;
    PyObject *num = NULL;
    PyObject *den = NULL;
    PyObject *f = NULL;

    ASSIGN_AND_CHECK_NULL(den, Rational_denominator_get(y, NULL));
    if (PyObject_RichCompareBool(den, PyONE, Py_EQ) == 1) {
        ASSIGN_AND_CHECK_NULL(num, Rational_numerator_get(y, NULL));
        ASSIGN_AND_CHECK_NULL(res, PyNumber_Power(x, num, Py_None));
    }
    else {
        ASSIGN_AND_CHECK_NULL(f, PyNumber_Float((PyObject *)y));
        ASSIGN_AND_CHECK_NULL(res, PyNumber_Power(x, f, Py_None));
    }
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(num);
    Py_XDECREF(den);
    Py_XDECREF(f);
    return res;
}

static PyObject *
Rational_pow(PyObject *x, PyObject *y, PyObject *mod) {
    if (mod != Py_None) {
        PyErr_SetString(PyExc_TypeError,
                        "3rd argument not allowed unless all arguments "
                        "are integers.");
        return NULL;
    }

    if (Rational_Check(x)) {
        if (PyObject_IsInstance(y, Real) ||
            PyObject_IsInstance(y, Decimal)) {
            return rn_pow_obj((RationalObject *)x, y);
        }
        Py_RETURN_NOTIMPLEMENTED;
    }
    else {          // y is a Rational
        return obj_pow_rn(x, (RationalObject *)y);
    }
}

// Converting methods

#define DEF_N_CONV_RND_MODE(rounding)                            \
    enum FPDEC_ROUNDING_MODE rnd = py_rnd_2_fpdec_rnd(rounding); \
    if (rnd > FPDEC_MAX_ROUNDING_MODE)                                               \
        goto ERROR

static PyObject *
Rational_adj_to_prec(RationalObject *self, PyObject *precision,
                     PyObject *rounding) {
    PyTypeObject *dec_type = Py_TYPE(self);
    RATIONAL_ALLOC_RESULT(dec_type);
    error_t rc;
    PyObject *pylong_prec = NULL;
    long prec;

/*
    if (precision == Py_None) {
        rc = fpdec_copy(&res->coeff, &self->coeff);
        CHECK_FPDEC_ERROR(rc);
        rc = fpdec_normalize_prec(&res->coeff);
        CHECK_FPDEC_ERROR(rc);
        goto CLEAN_UP;
    }

    if (PyLong_Check(precision)) { // NOLINT(hicpp-signed-bitwise)
        Py_INCREF(precision);
        pylong_prec = precision;
    }
    else if (PyObject_IsInstance(precision, Integral))
        ASSIGN_AND_CHECK_NULL(pylong_prec, PyNumber_Long(precision));
    else {
        PyErr_SetString(PyExc_TypeError,
                        "Precision must be of type 'Integral'.");
        goto ERROR;
    }
    prec = PyLong_AsLong(pylong_prec);
    if (PyErr_Occurred())
        goto ERROR;

    if (prec < -FPDEC_MAX_DEC_PREC || prec > FPDEC_MAX_DEC_PREC) {
        PyErr_Format(PyExc_ValueError, "Precision limit exceed: %ld", prec);
        goto ERROR;
    }

    DEF_N_CONV_RND_MODE(rounding);
    rc = fpdec_adjusted(&res->coeff, &self->coeff, prec, rnd);
    CHECK_FPDEC_ERROR(rc);
*/

    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());
    Py_CLEAR(res);

CLEAN_UP:
    Py_XDECREF(pylong_prec);
    return (PyObject *)res;
}

static PyObject *
Rational_adjusted(RationalObject *self, PyObject *args, PyObject *kwds) {
    static char *kw_names[] = {"precision", "rounding", NULL};
    PyObject *precision = Py_None;
    PyObject *rounding = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kw_names,
                                     &precision, &rounding))
        return NULL;

    return Rational_adj_to_prec(self, precision, rounding);
}

//static PyObject *
//RationalType_from_rational_obj(PyTypeObject *type, RationalObject *rn,
//                               rn_prec_t adjust_to_prec) {
//    if (adjust_to_prec == RN_UNLIM_PREC || rn->prec <= adjust_to_prec ||
//        (rn->variant == RN_FPDEC && rn->exp >= -adjust_to_prec)) {
//        if (type == RationalType) {
//            // rn is a direct instance of RationalType, a direct instance of
//            // RationalType is wanted and there's no need to adjust the result,
//            // so just return the given instance (ref count increased)
//            Py_INCREF(rn);
//            return (PyObject *)rn;
//        }
//        else {
//            RATIONAL_ALLOC_SELF(type);
//            Rational_raw_data_copy(self, rn);
//            if (rn->numerator != NULL) {
//                Py_INCREF(rn->numerator);
//                self->numerator = rn->numerator;
//                Py_INCREF(rn->denominator);
//                self->denominator = rn->denominator;
//            }
//            self->hash = rn->hash;
//            return (PyObject *)self;
//        }
//    }
//    // resulting value has to be adjusted
//    RATIONAL_ALLOC_SELF(type);
//    Rational_raw_data_copy(self, rn);
//    switch (rn->variant) {
//        case RN_FPDEC:
//            if (rnd_adjust_coeff_exp(&self->coeff, &self->exp,
//                                     self->sign == RN_SIGN_NEG,
//                                     adjust_to_prec) < 0)
//                goto FALLBACK;
//            break;
//        case RN_U64_QUOT:
//            if (rnq_adjust_quot(&self->u64_num, &self->u64_den,
//                                self->sign == RN_SIGN_NEG,
//                                adjust_to_prec) < 0)
//                goto FALLBACK;
//            if (rnd_from_quot(&self->coeff, &self->exp,
//                              self->u64_num, self->u64_den) == 0)
//                self->variant = RN_FPDEC;
//            break;
//        case RN_PYINT_QUOT: {
//            CHECK_RC(rnp_adjusted(RN_PYINT_QUOT_PTR(self),
//                                  RN_PYINT_QUOT_PTR(rn),
//                                  adjust_to_prec));
//            rn_optimize_pyquot(self);
//            break;
//        }
//        default:
//            PyErr_SetString(PyExc_RuntimeError,
//                            "Internal representation error");
//            goto ERROR;
//    }
//    self->prec = adjust_to_prec;
//    return (PyObject *)self;
//
//FALLBACK:
//    if (rn_assert_num_den(rn) == 0) {
//        CHECK_RC(rnp_adjusted(RN_PYINT_QUOT_PTR(self),
//                              RN_PYINT_QUOT_PTR(rn),
//                              adjust_to_prec));
//        rn_optimize_pyquot(self);
//        self->prec = adjust_to_prec;
//        return (PyObject *)self;
//    }
//
//ERROR:
//    Py_XDECREF(self);
//    return NULL;
//}

static PyObject *
Rational_quantize(RationalObject *self, PyObject *args, PyObject *kwds) {
    static char *kw_names[] = {"quant", "rounding", NULL};
    PyObject *quant = Py_None;
    PyObject *rounding = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kw_names, &quant,
                                     &rounding))
        return NULL;

    RATIONAL_ALLOC_RESULT(Py_TYPE(self));
    PyObject *frac_quant = NULL;
    PyObject *t = NULL;
    PyObject *num = NULL;
    PyObject *den = NULL;
/*
    fpdec_t *fpz = &dec->coeff;
    error_t rc;
    fpdec_t tmp_q = FPDEC_ZERO;
    fpdec_t *fp_quant;
    DEF_N_CONV_RND_MODE(rounding);

    fp_quant = fpdec_from_number(&tmp_q, quant);
    if (fp_quant == NULL) {
        if (PyErr_Occurred()) {
            PyErr_Format(PyExc_ValueError, "Can't quantize to '%R'.",
                         quant);
            goto ERROR;
        }
        else if (PyObject_IsInstance(quant, Real) ||
                 PyObject_IsInstance(quant, Decimal))
            goto FALLBACK;
        else {
            PyErr_Format(PyExc_TypeError, "Can't quantize to a '%S': %S.",
                         Py_TYPE(quant), quant);
            goto ERROR;
        }
    }
    rc = fpdec_quantized(fpz, &self->coeff, fp_quant, rnd);
    if (rc == FPDEC_OK)
        goto CLEAN_UP;

FALLBACK:
    fpdec_reset_to_zero(&dec->coeff, 0);
    res = NULL;
    ASSIGN_AND_CHECK_NULL(frac_quant,
                          PyObject_CallFunctionObjArgs(Fraction, quant, NULL));
    ASSIGN_AND_CHECK_NULL(num, PyObject_GetAttrString(frac_quant,
                                                      "numerator"));
    ASSIGN_AND_CHECK_NULL(den, PyObject_GetAttrString(frac_quant,
                                                      "denominator"));
    ASSIGN_AND_CHECK_NULL(t, Rational_mul((PyObject *)self, den));
    tmp_q = FPDEC_ZERO;
    rc = fpdec_from_pylong(&tmp_q, num);
    CHECK_FPDEC_ERROR(rc);
    rc = fpdec_div(&dec->coeff, &((RationalObject *)t)->coeff, &tmp_q, 0, rnd);
    CHECK_FPDEC_ERROR(rc);
    ASSIGN_AND_CHECK_NULL(res, Rational_mul((PyObject *)dec, frac_quant));
    Py_CLEAR(dec);
*/
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());
    Py_CLEAR(res);
    res = NULL;

CLEAN_UP:
    Py_XDECREF(frac_quant);
    Py_XDECREF(t);
    Py_XDECREF(num);
    Py_XDECREF(den);
    return (PyObject *)res;
}

static PyObject *
Rational_as_fraction(RationalObject *self, PyObject *args UNUSED) {
    PyObject *res = NULL;
    PyObject *num = NULL;
    PyObject *den = NULL;

    ASSIGN_AND_CHECK_NULL(num, Rational_numerator_get(self, NULL));
    ASSIGN_AND_CHECK_NULL(den, Rational_denominator_get(self, NULL));
    ASSIGN_AND_CHECK_NULL(res, PyObject_CallFunctionObjArgs(Fraction,
                                                            num, den, NULL));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(num);
    Py_XDECREF(den);
    return res;
}

static PyObject *
Rational_as_integer_ratio(RationalObject *self, PyObject *args UNUSED) {
    PyObject *res = NULL;
    PyObject *num = NULL;
    PyObject *den = NULL;

    ASSIGN_AND_CHECK_NULL(num, Rational_numerator_get(self, NULL));
    ASSIGN_AND_CHECK_NULL(den, Rational_denominator_get(self, NULL));
    ASSIGN_AND_CHECK_NULL(res, PyTuple_Pack(2, num, den));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(num);
    Py_XDECREF(den);
    return res;
}

static PyObject *
Rational_floor(RationalObject *self, PyObject *args UNUSED) {
    PyObject *res = NULL;
    PyObject *num = NULL;
    PyObject *den = NULL;

    ASSIGN_AND_CHECK_NULL(num, Rational_numerator_get(self, NULL));
    ASSIGN_AND_CHECK_NULL(den, Rational_denominator_get(self, NULL));
    ASSIGN_AND_CHECK_NULL(res, PyNumber_FloorDivide(num, den));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(num);
    Py_XDECREF(den);
    return res;
}

static PyObject *
Rational_ceil(RationalObject *self, PyObject *args UNUSED) {
    PyObject *res = NULL;
    PyObject *num = NULL;
    PyObject *den = NULL;
    PyObject *t = NULL;

    ASSIGN_AND_CHECK_NULL(num, Rational_numerator_get(self, NULL));
    ASSIGN_AND_CHECK_NULL(den, Rational_denominator_get(self, NULL));
    ASSIGN_AND_CHECK_NULL(t, PyNumber_Negative(num));
    ASSIGN_AND_CHECK_NULL(t, PyNumber_InPlaceFloorDivide(t, den));
    ASSIGN_AND_CHECK_NULL(res, PyNumber_Negative(t));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(num);
    Py_XDECREF(den);
    Py_XDECREF(t);
    return res;
}

static PyObject *
Rational_round(RationalObject *self, PyObject *args, PyObject *kwds) {
    static char *kw_names[] = {"precision", NULL};
    PyObject *precision = Py_None;
    PyObject *adj = NULL;
    PyObject *res = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kw_names,
                                     &precision))
        return NULL;

    if (precision == Py_None) {
        ASSIGN_AND_CHECK_NULL(adj, Rational_adj_to_prec(self, PyZERO,
                                                       Py_None));
        ASSIGN_AND_CHECK_NULL(res, Rational_int((RationalObject *)adj, NULL));
    }
    else
        ASSIGN_AND_CHECK_NULL(res, Rational_adj_to_prec(self, precision,
                                                       Py_None));
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_XDECREF(adj);
    return res;
}

// Pickle helper

/*
static PyObject *
Rational_setstate(RationalObject *self, PyObject *state) {
    char *buf = NULL;
    error_t rc;

    ASSIGN_AND_CHECK_NULL(buf, PyBytes_AsString(state));
    rc = fpdec_from_ascii_literal(&self->coeff, buf);
    CHECK_FPDEC_ERROR(rc);
    goto CLEAN_UP;

ERROR:
    assert(PyErr_Occurred());

CLEAN_UP:
    Py_INCREF(Py_None);
    return Py_None;
}
*/

// Rational type spec

static PyGetSetDef Rational_properties[] = {
    {"_prec", (getter)Rational_precision_get, 0,
     Rational_precision_doc, 0},
    {"magnitude", (getter)Rational_magnitude_get, 0,
     Rational_magnitude_doc, 0},
    {"numerator", (getter)Rational_numerator_get, 0,
     Rational_numerator_doc, 0},
    {"denominator", (getter)Rational_denominator_get, 0,
     Rational_denominator_doc, 0},
    {"real", (getter)Rational_real_get, 0,
     Rational_real_doc, 0},
    {"imag", (getter)Rational_imag_get, 0,
     Rational_imag_doc, 0},
    {0, 0, 0, 0, 0}};

static PyMethodDef Rational_methods[] = {
    /* class methods */
    {"from_float",
     (PyCFunction)RationalType_from_float_or_int,
     METH_O | METH_CLASS, // NOLINT(hicpp-signed-bitwise)
     RationalType_from_float_doc},
    {"from_decimal",
     (PyCFunction)RationalType_from_decimal_or_int,
     METH_O | METH_CLASS, // NOLINT(hicpp-signed-bitwise)
     RationalType_from_decimal_doc},
    // instance methods
    {"adjusted",
     (PyCFunction)(void *)(PyCFunctionWithKeywords)Rational_adjusted,
     METH_VARARGS | METH_KEYWORDS, // NOLINT(hicpp-signed-bitwise)
     Rational_adjusted_doc},
    {"quantize",
     (PyCFunction)(void *)(PyCFunctionWithKeywords)Rational_quantize,
     METH_VARARGS | METH_KEYWORDS, // NOLINT(hicpp-signed-bitwise)
     Rational_quantize_doc},
    {"as_fraction",
     (PyCFunction)Rational_as_fraction,
     METH_NOARGS,
     Rational_as_fraction_doc},
    {"as_integer_ratio",
     (PyCFunction)Rational_as_integer_ratio,
     METH_NOARGS,
     Rational_as_integer_ratio_doc},
    // special methods
    {"__copy__",
     (PyCFunction)Rational_copy,
     METH_NOARGS,
     Rational_copy_doc},
    {"__deepcopy__",
     (PyCFunction)Rational_deepcopy,
     METH_O,
     Rational_copy_doc},
    //{"__getstate__",
    // (PyCFunction)Rational_bytes,
    // METH_NOARGS,
    // Rational_getstate_doc},
    //{"__setstate__",
    // (PyCFunction)Rational_setstate,
    // METH_O,
    // Rational_setstate_doc},
    {"__bytes__",
     (PyCFunction)Rational_bytes,
     METH_NOARGS,
     Rational_bytes_doc},
    {"__format__",
     (PyCFunction)Rational_format,
     METH_O,
     Rational_format_doc},
    {"__trunc__",
     (PyCFunction)Rational_int,
     METH_NOARGS,
     Rational_trunc_doc},
    {"__floor__",
     (PyCFunction)Rational_floor,
     METH_NOARGS,
     Rational_floor_doc},
    {"__ceil__",
     (PyCFunction)Rational_ceil,
     METH_NOARGS,
     Rational_ceil_doc},
    {"__round__",
     (PyCFunction)(void *)(PyCFunctionWithKeywords)Rational_round,
     METH_VARARGS | METH_KEYWORDS, // NOLINT(hicpp-signed-bitwise)
     Rational_round_doc},
    {0, 0, 0, 0}
};

static PyType_Slot Rational_type_slots[] = {
    {Py_tp_doc, RationalType_doc},
    {Py_tp_new, RationalType_new},
    {Py_tp_dealloc, Rational_dealloc},
    {Py_tp_free, PyObject_Del},
    {Py_tp_richcompare, Rational_richcompare},
    {Py_tp_hash, Rational_hash},
    {Py_tp_str, Rational_str},
    {Py_tp_repr, Rational_repr},
    /* properties */
    {Py_tp_getset, Rational_properties},
    /* number methods */
    {Py_nb_bool, Rational_bool},
    {Py_nb_add, Rational_add},
    {Py_nb_subtract, Rational_sub},
    {Py_nb_multiply, Rational_mul},
    {Py_nb_remainder, Rational_remainder},
    {Py_nb_divmod, Rational_divmod},
    {Py_nb_power, Rational_pow},
    {Py_nb_negative, Rational_neg},
    {Py_nb_positive, Rational_pos},
    {Py_nb_absolute, Rational_abs},
    {Py_nb_int, Rational_int},
    {Py_nb_float, Rational_float},
    {Py_nb_floor_divide, Rational_floordiv},
    {Py_nb_true_divide, Rational_truediv},
    /* other methods */
    {Py_tp_methods, Rational_methods},
    {0, NULL}
};

static PyType_Spec RationalType_spec = {
    "rational.Rational",                    /* name */
    sizeof(RationalObject),                 /* basicsize */
    0,                                      /* itemsize */
    0,                                      /* flags */
    Rational_type_slots                     /* slots */
};

/*============================================================================
* rational module
* ==========================================================================*/

#define PYMOD_ADD_OBJ(module, name, obj)                    \
    do {                                                    \
        Py_INCREF(obj);                                     \
        if (PyModule_AddObject(module, name, obj) < 0) {    \
            Py_DECREF(obj);                                 \
            goto ERROR;                                     \
        }                                                   \
    } while (0)

PyDoc_STRVAR(rational_doc, "Rational number arithmetic.");

static int
rational_exec(PyObject *module) {
    int rc = 0;
    /* Import from numbers */
    PyObject *numbers = NULL;
    ASSIGN_AND_CHECK_NULL(numbers, PyImport_ImportModule("numbers"));
    ASSIGN_AND_CHECK_NULL(Number, PyObject_GetAttrString(numbers, "Number"));
    ASSIGN_AND_CHECK_NULL(Complex,
                          PyObject_GetAttrString(numbers, "Complex"));
    ASSIGN_AND_CHECK_NULL(Real, PyObject_GetAttrString(numbers, "Real"));
    ASSIGN_AND_CHECK_NULL(Rational,
                          PyObject_GetAttrString(numbers, "Rational"));
    ASSIGN_AND_CHECK_NULL(Integral,
                          PyObject_GetAttrString(numbers, "Integral"));
    /* Import from fractions */
    PyObject *fractions = NULL;
    ASSIGN_AND_CHECK_NULL(fractions, PyImport_ImportModule("fractions"));
    ASSIGN_AND_CHECK_NULL(Fraction,
                          PyObject_GetAttrString(fractions, "Fraction"));
    /* Import from decimal */
    PyObject *decimal = NULL;
    ASSIGN_AND_CHECK_NULL(decimal, PyImport_ImportModule("decimal"));
    ASSIGN_AND_CHECK_NULL(Decimal,
                          PyObject_GetAttrString(decimal, "Decimal"));
    /* Import from math */
    PyObject *math = NULL;
    ASSIGN_AND_CHECK_NULL(math, PyImport_ImportModule("math"));
    ASSIGN_AND_CHECK_NULL(PyNumber_gcd, PyObject_GetAttrString(math, "gcd"));
    /* Import from sys */
    PyObject *sys = NULL;
    PyObject *hash_info = NULL;
    ASSIGN_AND_CHECK_NULL(sys, PyImport_ImportModule("sys"));
    ASSIGN_AND_CHECK_NULL(hash_info,
                          PyObject_GetAttrString(sys, "hash_info"));
    ASSIGN_AND_CHECK_NULL(PyHASH_MODULUS,
                          PyObject_GetAttrString(hash_info, "modulus"));
    ASSIGN_AND_CHECK_NULL(PyHASH_INF,
                          PyObject_GetAttrString(hash_info, "inf"));
    /* PyLong methods */
    ASSIGN_AND_CHECK_NULL(PyLong_bit_length,
                          PyObject_GetAttrString((PyObject *)&PyLong_Type,
                                                 "bit_length"));
    /* Import from rounding */
    PyObject *rounding = NULL;
    ASSIGN_AND_CHECK_NULL(rounding,
                          PyImport_ImportModule("rational.rounding"));
    ASSIGN_AND_CHECK_NULL(Rounding,
                          PyObject_GetAttrString(rounding, "Rounding"));
    ASSIGN_AND_CHECK_NULL(get_dflt_rounding_mode,
                          PyObject_GetAttrString(rounding,
                                                 "get_dflt_rounding_mode"));

    /* Init global Python constants */
    ASSIGN_AND_CHECK_NULL(PyZERO, PyLong_FromLong(0L));
    ASSIGN_AND_CHECK_NULL(PyONE, PyLong_FromLong(1L));
    ASSIGN_AND_CHECK_NULL(PyTWO, PyLong_FromLong(2L));
    ASSIGN_AND_CHECK_NULL(PyFIVE, PyLong_FromLong(5L));
    ASSIGN_AND_CHECK_NULL(PyTEN, PyLong_FromLong(10L));
    ASSIGN_AND_CHECK_NULL(Py64, PyLong_FromLong(64L));
    ASSIGN_AND_CHECK_NULL(PyUInt64Max,
                          PyLong_FromUnsignedLongLong(UINT64_MAX));
    ASSIGN_AND_CHECK_NULL(Py2pow64, PyNumber_Lshift(PyONE, Py64));
    ASSIGN_AND_CHECK_NULL(PyHASH_MODULUS_2,
                          PyLong_FromLongLong(PyLong_AsLongLong(PyHASH_MODULUS)
                          - 2));

    /* Add types */
    ASSIGN_AND_CHECK_NULL(RationalType,
                          (PyTypeObject *)PyType_FromSpec(&RationalType_spec));
    PYMOD_ADD_OBJ(module, "Rational", (PyObject *)RationalType);

    /* Register RationalType as Rational */
    ASSIGN_AND_CHECK_NULL(RationalType,
                          (PyTypeObject *)PyObject_CallMethod(Rational,
                                                              "register", "O",
                                                              RationalType));

    goto CLEAN_UP;

ERROR:
    Py_CLEAR(Number);
    Py_CLEAR(Complex);
    Py_CLEAR(Real);
    Py_CLEAR(Rational);
    Py_CLEAR(Integral);
    Py_CLEAR(Fraction);
    Py_CLEAR(Decimal);
    Py_CLEAR(RationalType);
    Py_CLEAR(Rounding);
    Py_CLEAR(get_dflt_rounding_mode);
    Py_CLEAR(PyNumber_gcd);
    Py_CLEAR(PyLong_bit_length);
    Py_CLEAR(PyZERO);
    Py_CLEAR(PyONE);
    Py_CLEAR(PyTWO);
    Py_CLEAR(PyFIVE);
    Py_CLEAR(PyTEN);
    Py_CLEAR(Py64);
    Py_CLEAR(PyUInt64Max);
    Py_CLEAR(Py2pow64);
    Py_CLEAR(PyHASH_MODULUS);
    Py_CLEAR(PyHASH_MODULUS_2);
    Py_CLEAR(PyHASH_INF);
    rc = -1;

CLEAN_UP:
    Py_CLEAR(numbers);
    Py_CLEAR(fractions);
    Py_CLEAR(decimal);
    Py_CLEAR(math);
    Py_CLEAR(sys);
    Py_CLEAR(hash_info);
    Py_CLEAR(rounding);
    return rc;
}

static PyModuleDef_Slot rational_slots[] = {
    {Py_mod_exec, rational_exec},
    {0, NULL}
};

static struct PyModuleDef rational_module = {
    PyModuleDef_HEAD_INIT,              /* m_base */
    "rational",                         /* m_name */
    rational_doc,                       /* m_doc */
    0,                                  /* m_size */
    NULL,                               /* m_methods */
    rational_slots,                     /* m_slots */
    NULL,                               /* m_traverse */
    NULL,                               /* m_clear */
    NULL                                /* m_free */
};

PyMODINIT_FUNC
PyInit_rational(void) {
    return PyModuleDef_Init(&rational_module);
}
