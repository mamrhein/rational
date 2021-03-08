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
                          "14/900"),
                         ids=("compact", "large", "fraction"))
def test_true(value):
    q = Rational(value)
    assert q


@pytest.mark.parametrize("value", (None, "0.0000", (0, -999999999)),
                         ids=("None", "0", "0/-999999999"))
def test_false(value):
    if isinstance(value, tuple):
        q = Rational(*value)
    else:
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


@pytest.mark.parametrize(("value", "str_"),
                         ((None, "0"),
                          (15, "15"),
                          ("17.50", "17.5"),
                          ("-20.7e-3", "-0.0207"),
                          ("0.0000000000207", "0.0000000000207"),
                          ("-319e-27", "-0." + "0" * 24 + "319"),
                          (887 * 10 ** 14, "887" + "0" * 14),
                          ("27e23", "27" + "0" * 23),
                          ("-287/8290", "-287/8290")),
                         ids=lambda p: str(p))
def test_str(value, str_):
    q = Rational(value)
    assert str(q) == str_


@pytest.mark.parametrize(("value", "bstr"),
                         ((None, b"0"),
                          ("-20.7e-3", b"-0.0207"),
                          ("0.0000000000207", b"0.0000000000207"),
                          (887 * 10 ** 14, b"887" + b"0" * 14),
                          ("-287/8290", b"-287/8290")),
                         ids=lambda p: str(p))
def test_bytes(value, bstr):
    q = Rational(value)
    assert bytes(q) == bstr


@pytest.mark.parametrize(("value", "repr_"),
                         ((None, "Rational(0)"),
                          ("15", "Rational(15)"),
                          ("15.000", "Rational(15)"),
                          ("15.400", "Rational('15.4')"),
                          ("-20.7e-3", "Rational('-0.0207')"),
                          ("0.0000000000207", "Rational('0.0000000000207')"),
                          (887 * 10 ** 14, "Rational(887" + "0" * 14 + ")"),
                          ("27/63", "Rational(3, 7)"),
                          ("-287/8290", "Rational(-287, 8290)"),
                          ("12345678901234567890123456/1234567",
                           "Rational(12345678901234567890123456, 1234567)"),),
                         ids=lambda p: str(p))
def test_repr(value, repr_):
    q = Rational(value)
    assert repr(q) == repr_
