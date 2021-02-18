# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------------
# Author:      Michael Amrhein (michael@adrhinum.de)
#
# Copyright:   (c) 2021 ff. Michael Amrhein
# License:     This program is part of a larger application. For license
#              details please read the file LICENSE.TXT provided together
#              with the application.
# ----------------------------------------------------------------------------
# $Source$
# $Revision$


"""Test driver for package 'rational' (conversions)."""
import math
from fractions import Fraction

import pytest

from rational import Rational


@pytest.mark.parametrize("value",
                         ("17.8",
                          ".".join(("1" * 3297, "4" * 33)),
                          "0.00014"),
                         ids=("compact", "large", "fraction"))
def test_true(value):
    q = Rational(value)
    assert q


@pytest.mark.parametrize("value", (None, "0.0000"),
                         ids=("None", "0"))
def test_false(value):
    q = Rational(value)
    assert not q


@pytest.mark.parametrize("value",
                         ("0.000",
                          "-17.03",
                          Fraction(9 ** 394, 10 ** 247),
                          Fraction(-19, 4000)),
                         ids=("zero", "compact", "large", "fraction"))
def test_int(value):
    f = Fraction(value)
    q = Rational(value)
    assert int(f) == int(q)


@pytest.mark.parametrize("value",
                         ("0.00000",
                          17,
                          "-33000.17",
                          Fraction(9 ** 394, 10 ** 247),
                          Fraction(-19, 400000)),
                         ids=("zero", "int", "compact", "large", "fraction"))
@pytest.mark.parametrize("func",
                         (math.trunc, math.floor, math.ceil),
                         ids=("trunc", "floor", "ceil"))
def test_math_funcs(func, value):
    f = Fraction(value)
    q = Rational(value)
    assert func(f) == func(q)


@pytest.mark.parametrize(("num", "den"),
                         ((17, 1),
                          (9 ** 394, 10 ** 247),
                          (-190, 400000)),
                         ids=("compact", "large", "fraction"))
def test_to_float(num, den):
    f = Fraction(num, den)
    q = Rational(num, den)
    assert float(f) == float(q)


@pytest.mark.parametrize("value",
                         ("0.00000",
                          17,
                          "-33000.17",
                          Fraction(9 ** 394, 10 ** 247),
                          Fraction(-19, 400000)),
                         ids=("zero", "int", "compact", "large", "fraction"))
def test_as_integer_ratio(value):
    f = Fraction(value)
    q = Rational(value)
    assert q.as_integer_ratio() == (f.numerator, f.denominator)


@pytest.mark.parametrize("value",
                         ("0.00000",
                          17,
                          "-33000.17",
                          Fraction(9 ** 394, 10 ** 247),
                          Fraction(-19, 400000)),
                         ids=("zero", "int", "compact", "large", "fraction"))
def test_as_fraction(value):
    f = Fraction(value)
    q = Rational(value)
    assert q.as_fraction() == f


@pytest.mark.parametrize(("value", "prec", "str_"),
                         ((None, None, "0"),
                          (None, 2, "0.00"),
                          ("-20.7e-3", 5, "-0.02070"),
                          ("0.0000000000207", None, "0.0000000000207"),
                          (887 * 10 ** 14, 0, "887" + "0" * 14)))
def test_str(value, prec, str_):
    q = Rational(value, precision=prec)
    assert str(q) == str_


@pytest.mark.parametrize(("value", "prec", "bstr"),
                         ((None, None, b"0"),
                          (None, 2, b"0.00"),
                          ("-20.7e-3", 5, b"-0.02070"),
                          ("0.0000000000207", None, b"0.0000000000207"),
                          (887 * 10 ** 14, 0, b"887" + b"0" * 14)))
def test_bytes(value, prec, bstr):
    q = Rational(value, precision=prec)
    assert bytes(q) == bstr


@pytest.mark.parametrize(("value", "prec", "repr_"),
                         ((None, None, "Rational(0)"),
                          (None, 2, "Rational(0, 2)"),
                          ("15", 2, "Rational(15, 2)"),
                          ("15.4", 2, "Rational('15.4', 2)"),
                          ("-20.7e-3", 5, "Rational('-0.0207', 5)"),
                          ("0.0000000000207", None,
                           "Rational('0.0000000000207')"),
                          (887 * 10 ** 14, 0,
                           "Rational(887" + "0" * 14 + ")")))
def test_repr(value, prec, repr_):
    q = Rational(value, precision=prec)
    assert repr(q) == repr_
