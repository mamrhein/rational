#!/usr/bin/env python
# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------------
# Copyright:   (c) 2018 ff. Michael Amrhein (michael@adrhinum.de)
# License:     This program is part of a larger application. For license
#              details please read the file LICENSE.TXT provided together
#              with the application.
# ----------------------------------------------------------------------------
# $Source$
# $Revision$


"""Test driver for package 'rational' (constructors)."""

import copy
from decimal import Decimal
from fractions import Fraction
from numbers import Integral
import sys

import pytest

from rational import (
    Rational, Rounding, get_dflt_rounding_mode, set_dflt_rounding_mode)

RN_MAX_PREC = 2 ** 63 - 1
RN_HUGE_PREC = 2 ** 16 - 1


@pytest.fixture(scope="module")
def dflt_rounding():
    rnd = get_dflt_rounding_mode()
    set_dflt_rounding_mode(Rounding.ROUND_HALF_UP)
    yield
    set_dflt_rounding_mode(rnd)


# noinspection PyUnusedLocal
def test_dflt_rounding(dflt_rounding):
    """Activate fixture to set default rounding"""
    pass


class IntWrapper:

    def __init__(self, i):
        assert isinstance(i, int)
        self.i = i

    def __int__(self):
        """int(self)"""
        return self.i

    __index__ = __int__

    def __eq__(self, i):
        """self == i"""
        return self.i == i


# noinspection PyUnresolvedReferences
Integral.register(IntWrapper)


class FloatWrapper:

    def __init__(self, f):
        assert isinstance(f, float)
        self.f = f

    def __float__(self):
        """float(self)"""
        return self.f

    def __eq__(self, f):
        """self == f"""
        return self.f == f


class Dummy:

    def __init__(self, s):
        self.s = str(s)

    def __float__(self):
        return float(len(self.s))

    def __int__(self):
        return len(self.s)


@pytest.mark.parametrize("prec", [None, 0, 7],
                         ids=("prec=None", "prec=0", "prec=7"))
def test_rational_no_value(prec):
    rn = Rational(precision=prec)
    assert isinstance(rn, Rational)
    assert rn.precision == (prec if prec else 0)


@pytest.mark.parametrize("num", [float, 3 + 2j, Dummy(17)],
                         ids=("num=float", "num=3+2j", "num=Dummy(17)"))
def test_rational_wrong_num_type(num):
    with pytest.raises(TypeError):
        Rational(num)


# noinspection PyTypeChecker
@pytest.mark.parametrize("prec", ["5", 7.5],
                         ids=("prec='5'", "prec=7.5"))
def test_rational_wrong_precision_type(prec):
    with pytest.raises(TypeError):
        Rational(precision=prec)


@pytest.mark.parametrize("prec", [-RN_MAX_PREC - 1, RN_MAX_PREC + 1],
                         ids=("prec<RN_MIN_PREC", "prec>RN_MAX_PREC"))
def test_rational_wrong_precision_value(prec):
    with pytest.raises(ValueError):
        Rational(precision=prec)


compact_coeff = 174
compact_prec = 1
compact_ratio = Fraction(compact_coeff, 10 ** compact_prec)
compact_str = ".174e2"
compact_adj = 0
compact_adj_ratio = round(compact_ratio)
small_coeff = 123456789012345678901234567890
small_prec = 20
small_ratio = Fraction(-small_coeff, 10 ** small_prec)
small_str = "-12345678901234567890.1234567890E-10"
small_adj = 15
small_adj_ratio = Fraction(round(-small_coeff, small_adj - small_prec),
                           10 ** small_prec)
large_coeff = 294898 * 10 ** 2453 + 1498953
large_prec = 459
large_ratio = Fraction(large_coeff, 10 ** large_prec)
large_str = f"{large_coeff}e-{large_prec}"
large_adj = large_prec - 30
large_adj_ratio = Fraction(round(large_coeff, large_adj - large_prec),
                           10 ** large_prec)


@pytest.mark.parametrize(("prec", "num", "den"),
                         ((compact_prec, compact_ratio.numerator,
                           compact_ratio.denominator),
                          (small_prec, -small_ratio.numerator,
                           small_ratio.denominator),
                          (large_prec, large_ratio.numerator,
                           -large_ratio.denominator),
                          (3, 8290, -10000)),
                         ids=("compact", "small", "large", "frac-only"))
def test_rational_from_ratio_dflt_prec(prec, num, den):
    rn = Rational(num, den)
    assert isinstance(rn, Rational)
    # assert rn.precision == prec
    f = Fraction(num, den)
    assert rn.numerator == f.numerator
    assert rn.denominator == f.denominator


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((compact_str, compact_prec, compact_ratio),
                          (small_str, small_prec, small_ratio),
                          (large_str, large_prec, large_ratio),
                          (" .829  ", 3, Fraction(829, 1000)),
                          ("\t -00000000 ", 0, 0),
                          ("\t -000.00000", 5, 0)),
                         ids=("compact", "small", "large", "frac-only",
                              "zeros", "zeros-with-point"))
def test_rational_from_str_dflt_prec(value, prec, ratio):
    rn = Rational(value)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((compact_str, compact_adj, compact_adj_ratio),
                          (small_str, small_adj, small_adj_ratio),
                          (large_str, large_adj, large_adj_ratio),
                          ("27.81029", IntWrapper(3), Fraction(2781, 100)),
                          (".829", 2, Fraction(83, 100)),
                          (".726", 0, 1)),
                         ids=("compact", "small", "large", "Integral as prec",
                              "frac-only", "carry-over"))
def test_rational_from_str_adj(value, prec, ratio):
    rn = Rational(value, precision=prec)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((compact_str, compact_prec, compact_ratio),
                          (small_str, small_prec, small_ratio),
                          (large_str, large_prec, large_ratio),
                          (" .829  ", 3, Fraction(829, 1000)),
                          ("\t -00000000 ", 0, 0),
                          ("\t -000.00000", 5, 0)),
                         ids=("compact", "small", "large", "frac-only",
                              "zeros", "zeros-with-point"))
def test_rational_from_str_no_adj(value, prec, ratio):
    prec *= 3
    rn = Rational(value, precision=prec)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize("value", ["\u1811\u1817.\u1814", "\u0f20.\u0f24"],
                         ids=["mongolian", "tibetian"])
def test_rational_from_non_ascii_digits(value):
    rn = Rational(value)
    assert isinstance(rn, Rational)


@pytest.mark.parametrize("value",
                         (" 1.23.5", "1.24e", "--4.92", "", "   ", "3,49E-3",
                          "\t+   \r\n"),
                         ids=("two-points", "missing-exp", "double-sign",
                              "empty-string", "blanks", "invalid-char",
                              "sign-only"))
def test_rational_from_str_wrong_format(value):
    with pytest.raises(ValueError):
        Rational(value)


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((compact_str, compact_prec, compact_ratio),
                          (small_str, small_prec, small_ratio),
                          (large_str, large_prec, large_ratio)),
                         ids=("compact", "small", "large"))
def test_rational_from_rational_dflt_prec(value, prec, ratio):
    rn = Rational(value)
    rn = Rational(rn)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((compact_str, compact_adj, compact_adj_ratio),
                          (small_str, small_adj, small_adj_ratio),
                          (large_str, large_adj, large_adj_ratio)),
                         ids=("compact", "small", "large"))
def test_rational_from_rational_adj(value, prec, ratio):
    rn = Rational(value)
    assert isinstance(rn, Rational)
    rn = Rational(rn, precision=prec)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((compact_str, compact_prec, compact_ratio),
                          (small_str, small_prec, small_ratio),
                          (large_str, large_prec, large_ratio)),
                         ids=("compact", "small", "large"))
def test_rational_from_rational_no_adj(value, prec, ratio):
    prec += 17
    rn = Rational(value)
    rn = Rational(rn, precision=prec)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((Decimal("123.4567"), 4,
                           Fraction(1234567, 10000)),
                          (5, 0, Fraction(5, 1))),
                         ids=("Decimal", "int"))
def test_rational_from_decimal_cls_meth(value, prec, ratio):
    rn = Rational.from_decimal(value)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize("value",
                         (Fraction(12346, 100), FloatWrapper(328.5), 5.31),
                         ids=("Fraction", "FloatWrapper", "float"))
def test_rational_from_decimal_cls_meth_wrong_type(value):
    with pytest.raises(TypeError):
        Rational.from_decimal(value)


@pytest.mark.parametrize(("value", "ratio"),
                         ((compact_coeff, Fraction(compact_coeff, 1)),
                          (small_coeff, Fraction(small_coeff, 1)),
                          (large_coeff, Fraction(large_coeff, 1)),
                          (IntWrapper(328), Fraction(328, 1))),
                         ids=("compact", "small", "large", "IntWrapper"))
def test_rational_from_integral(value, ratio):
    rn = Rational(value)
    assert isinstance(rn, Rational)
    assert rn.precision == 0
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((compact_coeff, compact_adj, compact_coeff),
                          (IntWrapper(328), 7, Fraction(328, 1)),
                          (19, 12, 19),
                          (small_coeff, small_adj, Fraction(small_coeff, 1)),
                          (large_coeff, large_adj, Fraction(large_coeff, 1)),
                          ),
                         ids=("compact", "IntWrapper", "prec > 9", "small",
                              "large",))
def test_rational_from_integral_adj(value, prec, ratio):
    rn = Rational(value, precision=prec)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((Decimal(compact_str), compact_prec,
                           compact_ratio),
                          (Decimal(small_str), small_prec, small_ratio),
                          (Decimal(large_str), large_prec, large_ratio),
                          (Decimal("5.4e6"), 0, Fraction(5400000, 1))),
                         ids=("compact", "small", "large", "pos-exp"))
def test_rational_from_decimal_dflt_prec(value, prec, ratio):
    rn = Rational(value)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((Decimal(compact_str), compact_adj,
                           compact_adj_ratio),
                          (Decimal(small_str), small_adj,
                           small_adj_ratio),
                          (Decimal(large_str), large_adj,
                           large_adj_ratio),
                          (Decimal("54e-3"), 3, Fraction(54, 1000)),
                          (Decimal("5.4e4"), 2, Fraction(54000, 1))),
                         ids=("compact", "small", "large", "exp+prec=0",
                              "pos-exp"))
def test_rational_from_decimal_adj(value, prec, ratio):
    rn = Rational(value, precision=prec)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((Decimal(compact_str), compact_prec,
                           compact_ratio),
                          (Decimal(small_str), small_prec, small_ratio),
                          (Decimal(large_str), large_prec, large_ratio),
                          (Decimal("5.4e6"), 0, Fraction(5400000, 1))),
                         ids=("compact", "small", "large", "pos-exp"))
def test_rational_from_decimal_no_adj(value, prec, ratio):
    prec *= 5
    rn = Rational(value, precision=prec)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec"),
                         ((Decimal('inf'), compact_prec),
                          (Decimal('-inf'), None),
                          (Decimal('nan'), large_prec)),
                         ids=("inf", "-inf", "nan"))
def test_rational_from_incompat_decimal(value, prec):
    with pytest.raises(ValueError):
        Rational(value, precision=prec)


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((17.5, 1, Fraction(175, 10)),
                          (sys.float_info.max, 0,
                           Fraction(int(sys.float_info.max), 1)),
                          (FloatWrapper(328.5), 1, Fraction(3285, 10))),
                         ids=("compact", "float.max", "FloatWrapper"))
def test_rational_from_float_dflt_prec(value, prec, ratio):
    rn = Rational(value)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((17.5, 3, Fraction(175, 10)),
                          (sys.float_info.min, 17, Fraction(0, 1)),
                          (FloatWrapper(328.5), 0, Fraction(329, 1))),
                         ids=("compact", "float.min", "FloatWrapper"))
def test_rational_from_float_adj(value, prec, ratio):
    rn = Rational(value, precision=prec)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((17.5, 1, Fraction(175, 10)),
                          (sys.float_info.max, 0,
                           Fraction(int(sys.float_info.max), 1)),
                          (FloatWrapper(328.5), 1, Fraction(3285, 10))),
                         ids=("compact", "float.max", "FloatWrapper"))
def test_rational_from_float_no_adj(value, prec, ratio):
    prec += 7
    rn = Rational(value, precision=prec)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec"),
                         ((float('inf'), compact_prec),
                          (float('-inf'), None),
                          (float('nan'), large_prec),),
                         ids=("inf", "-inf", "nan",))
def test_rational_from_incompat_float(value, prec):
    with pytest.raises(ValueError):
        Rational(value, precision=prec)


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((1.5, 1, Fraction(15, 10)),
                          (sys.float_info.max, 0,
                           Fraction(int(sys.float_info.max), 1)),
                          (5, 0, Fraction(5, 1))),
                         ids=("compact", "float.max", "int"))
def test_rational_from_float_cls_meth(value, prec, ratio):
    rn = Rational.from_float(value)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize("value",
                         (Fraction(12346, 100),
                          FloatWrapper(328.5),
                          Decimal("5.31")),
                         ids=("Fraction", "FloatWrapper", "Decimal"))
def test_rational_from_float_cls_meth_wrong_type(value):
    with pytest.raises(TypeError):
        Rational.from_float(value)


@pytest.mark.parametrize(("prec", "ratio"),
                         ((1, Fraction(175, 10)),
                          (0, Fraction(int(sys.float_info.max), 1)),
                          (15, Fraction(sys.maxsize, 10 ** 15)),
                          (63, Fraction(1, sys.maxsize + 1))),
                         ids=("compact", "float.max", "maxsize", "frac_only"))
def test_rational_from_fraction_dflt_prec(prec, ratio):
    rn = Rational(ratio)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((Fraction(175, 10), 0, Fraction(18, 1)),
                          (Fraction(int(sys.float_info.max), 1), 7,
                           Fraction(int(sys.float_info.max), 1)),
                          (Fraction(sys.maxsize, 10 ** 15), 10,
                           Fraction(round(sys.maxsize, -5), 10 ** 15)),
                          (Fraction(1, 333333333333333333333333333333), 30,
                           round(Fraction(1, 333333333333333333333333333333),
                                 30))),
                         ids=("compact", "float.max", "maxsize", "fraction"))
def test_rational_from_fraction_adj(value, prec, ratio):
    rn = Rational(value, precision=prec)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("prec", "ratio"),
                         ((1, Fraction(175, 10)),
                          (0, Fraction(int(sys.float_info.max), 1)),
                          (15, Fraction(sys.maxsize, 10 ** 15)),
                          (63, Fraction(1, sys.maxsize + 1))),
                         ids=("compact", "float.max", "maxsize", "frac_only"))
def test_rational_from_fraction_no_adj(prec, ratio):
    prec = 2 * prec + 5
    rn = Rational(ratio, precision=prec)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


# @pytest.mark.parametrize(("value", "prec"),
#                          ((Fraction(1, 3), None),),
#                          ids=("1/3",))
# def test_rational_from_incompat_fraction(value, prec):
#     with pytest.raises(ValueError):
#         Rational(value, precision=prec)


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((17.5, 1, Fraction(175, 10)),
                          (Fraction(1, 3), RN_HUGE_PREC,
                           Fraction(int("3" * RN_HUGE_PREC),
                                    10 ** RN_HUGE_PREC)),
                          (Fraction(328, 100000), 5, Fraction(328, 100000))),
                         ids=("float", "1/3", "Fraction"))
def test_rational_from_real_cls_meth_non_exact(value, prec, ratio):
    rn = Rational.from_real(value, exact=False)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((17.5, 1, Fraction(175, 10)),
                          (sys.float_info.max, 0,
                           Fraction(int(sys.float_info.max), 1)),
                          (Fraction(3285, 10), 1, Fraction(3285, 10))),
                         ids=("float", "float.max", "Fraction"))
def test_rational_from_real_cls_meth_exact(value, prec, ratio):
    rn = Rational.from_real(value, exact=True)
    assert isinstance(rn, Rational)
    assert rn.precision == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize("exact",
                         (False, True),
                         ids=("non-exact", "exact"))
@pytest.mark.parametrize("value",
                         (float("inf"), float("nan")),
                         ids=("inf", "nan"))
def test_rational_from_real_cls_meth_incompat_value(value, exact):
    with pytest.raises(ValueError):
        Rational.from_real(value, exact=exact)


@pytest.mark.parametrize("value",
                         (3 + 2j, "31.209", FloatWrapper(328.5)),
                         ids=("complex", "str", "FloatWrapper"))
def test_rational_from_real_cls_meth_wrong_type(value):
    with pytest.raises(TypeError):
        Rational.from_real(value)


@pytest.mark.parametrize("value",
                         ("17.800",
                          ".".join(("1" * 3097, "4" * 33 + "0" * 19)),
                          "-0.00014"),
                         ids=("compact", "large", "fraction"))
def test_copy(value):
    rn = Rational(value)
    assert copy.copy(rn) is rn
    assert copy.deepcopy(rn) is rn


# @pytest.mark.parametrize("value",
#                          ("-0." + "7" * (RN_MAX_PREC + 1),),
#                          ids=(f"prec > {RN_MAX_PREC}",))
# def test_limits_exceeded(value):
#     with pytest.raises(ValueError):
#         Rational(value)
