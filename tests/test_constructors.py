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
from numbers import Integral, Real
import sys

import pytest

from rational import (
    Rational, Rounding, get_dflt_rounding_mode, set_dflt_rounding_mode)


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

    def as_integer_ratio(self):
        """Return integer ratio equal to `self`."""
        return self.f.as_integer_ratio()


# noinspection PyUnresolvedReferences
Real.register(FloatWrapper)


class Dummy:

    def __init__(self, s):
        self.s = str(s)

    def __float__(self):
        return float(len(self.s))

    def __int__(self):
        return len(self.s)


def test_rational_no_value():
    rn = Rational()
    assert isinstance(rn, Rational)
    assert rn._prec == 0


@pytest.mark.parametrize("num", [float, 3 + 2j, Dummy(17)],
                         ids=("num=float", "num=3+2j", "num=Dummy(17)"))
def test_rational_wrong_num_type(num):
    with pytest.raises(TypeError):
        Rational(num)


compact_coeff = 174
compact_prec = 1
compact_ratio = Fraction(compact_coeff, 10 ** compact_prec)
compact_str = ".174e2"
small_coeff = 123456789012345678901234567890
small_prec = 20
small_ratio = Fraction(-small_coeff, 10 ** small_prec)
small_str = "-12345678901234567890.1234567890E-10"
large_coeff = 294898 * 10 ** 2453 + 1498953
large_prec = 459
large_ratio = Fraction(large_coeff, 10 ** large_prec)
large_str = f"{large_coeff}e-{large_prec}"


@pytest.mark.parametrize(("num", "den"),
                         ((compact_ratio.numerator, compact_ratio.denominator),
                          (small_ratio.numerator, small_ratio.denominator),
                          (large_ratio.numerator, -large_ratio.denominator),
                          (8290, -10000),
                          (small_coeff, large_coeff),
                          (large_coeff, small_coeff),
                          (-compact_coeff, -large_ratio),
                          (-small_ratio, -compact_coeff),
                          (compact_ratio, -large_ratio),
                          (-small_ratio, compact_ratio),),
                         ids=("compact", "small", "large", "frac-only",
                              "small/large", "large/small",
                              "compact/large-ratio",
                              "small-ratio/compact",
                              "compact-ratio/large-ratio",
                              "small-ratio/compact-ratio"))
def test_rational_from_ratio(num, den):
    rn = Rational(num, den)
    assert isinstance(rn, Rational)
    f = Fraction(num, den)
    assert rn.numerator == f.numerator
    assert rn.denominator == f.denominator


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((compact_str, compact_prec, compact_ratio),
                          (small_str, small_prec, small_ratio),
                          (large_str, None, large_ratio),
                          (" .829  ", 3, Fraction(829, 1000)),
                          ("\t -00000000 ", 0, 0),
                          ("\t -000.00000", 0, 0),
                          (" -1/3\t ", None, Fraction(-1, 3)),),
                         ids=("compact", "small", "large", "frac-only",
                              "zeros", "zeros-with-point", "1/3",))
def test_rational_from_str(value, prec, ratio):
    rn = Rational(value)
    assert isinstance(rn, Rational)
    # assert rn._prec == prec
    assert rn.numerator == ratio.numerator
    assert rn.denominator == ratio.denominator


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


@pytest.mark.parametrize("ratio",
                         (compact_ratio, small_ratio, large_ratio),
                         ids=("compact", "small", "large"))
def test_rational_from_rational(ratio):
    rn = Rational(ratio)
    rn = Rational(rn)
    assert isinstance(rn, Rational)
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((Decimal("123.4567"), 4,
                           Fraction(1234567, 10000)),
                          (5, 0, Fraction(5, 1))),
                         ids=("Decimal", "int"))
def test_rational_from_decimal_cls_meth(value, prec, ratio):
    rn = Rational.from_decimal(value)
    assert isinstance(rn, Rational)
    # assert rn._prec == prec
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
    # assert rn._prec == 0
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((Decimal(compact_str), compact_prec,
                           compact_ratio),
                          (Decimal(small_str), small_prec, small_ratio),
                          (Decimal(large_str), large_prec, large_ratio),
                          (Decimal("5.4e6"), 0, Fraction(5400000, 1))),
                         ids=("compact", "small", "large", "pos-exp"))
def test_rational_from_decimal(value, prec, ratio):
    rn = Rational(value)
    assert isinstance(rn, Rational)
    # assert rn._prec == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize("value",
                         (Decimal('inf'), Decimal('-inf'), Decimal('nan')),
                         ids=("inf", "-inf", "nan"))
def test_rational_from_incompat_decimal(value):
    with pytest.raises(ValueError):
        Rational(value)


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((17.5, 1, Fraction(175, 10)),
                          (sys.float_info.max, 0,
                           Fraction(int(sys.float_info.max), 1)),
                          (FloatWrapper(328.5), 1, Fraction(3285, 10))),
                         ids=("compact", "float.max", "FloatWrapper"))
def test_rational_from_float(value, prec, ratio):
    rn = Rational(value)
    assert isinstance(rn, Rational)
    # assert rn._prec == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize("value",
                         (float('inf'), float('-inf'), float('nan')),
                         ids=("inf", "-inf", "nan"))
def test_rational_from_incompat_float(value):
    with pytest.raises(ValueError):
        Rational(value)


@pytest.mark.parametrize(("value", "prec", "ratio"),
                         ((1.5, 1, Fraction(15, 10)),
                          (sys.float_info.max, 0,
                           Fraction(int(sys.float_info.max), 1)),
                          (5, 0, Fraction(5, 1))),
                         ids=("compact", "float.max", "int"))
def test_rational_from_float_cls_meth(value, prec, ratio):
    rn = Rational.from_float(value)
    assert isinstance(rn, Rational)
    # assert rn._prec == prec
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
def test_rational_from_fraction(prec, ratio):
    rn = Rational(ratio)
    assert isinstance(rn, Rational)
    # assert rn._prec == prec
    assert rn.as_fraction() == ratio


@pytest.mark.parametrize("value",
                         ("17.800",
                          ".".join(("1" * 3097, "4" * 33 + "0" * 19)),
                          "-0.00014"),
                         ids=("compact", "large", "fraction"))
def test_copy(value):
    rn = Rational(value)
    assert copy.copy(rn) is rn
    assert copy.deepcopy(rn) is rn


@pytest.mark.parametrize(("num", "den", "n_digits"),
                         ((compact_ratio.numerator, compact_ratio.denominator,
                           None),
                          (small_ratio.numerator, small_ratio.denominator,
                           None),
                          (large_ratio.numerator, -large_ratio.denominator, 2),
                          (8290, -10000, -2),
                          (small_coeff, large_coeff, 5),
                          (large_coeff, small_coeff, 0),
                          (-compact_coeff, -large_ratio, 4),
                          (-small_ratio, -compact_coeff, 0),
                          (compact_ratio, -large_ratio, 20),
                          (-small_ratio, compact_ratio, -3),),
                         ids=("compact", "small", "large", "frac-only",
                              "small/large", "large/small",
                              "compact/large-ratio",
                              "small-ratio/compact",
                              "compact-ratio/large-ratio",
                              "small-ratio/compact-ratio"))
def test_rational_rounded(with_round_half_even, num, den, n_digits):
    rn = Rational.rounded(num, den, n_digits)
    assert isinstance(rn, Rational)
    f = round(Fraction(num, den), n_digits)
    assert rn.numerator == f.numerator
    assert rn.denominator == f.denominator
