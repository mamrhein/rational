# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------------
# Copyright:   (c) 2021 ff. Michael Amrhein (michael@adrhinum.de)
# License:     This program is part of a larger application. For license
#              details please read the file LICENSE.TXT provided together
#              with the application.
# ----------------------------------------------------------------------------
# $Source$
# $Revision$

"""Test driver for package 'rational' (properties)."""

from fractions import Fraction

import pytest

from rational import Rational


@pytest.mark.parametrize(("value", "magn"),
                         (("17.8", 1),
                          (".".join(("1" * 3297, "4" * 33)), 3296),
                          ("-0.00014", -4),
                          (0.4, -1),
                          ("0.1", -1),
                          ((1, 33), -2)),
                         ids=("compact", "large", "fraction", "0.4", "0.1",
                              "1/33"))
def test_magnitude(value, magn):
    if isinstance(value, tuple):
        rn = Rational(*value)
    else:
        rn = Rational(value)
    assert rn.magnitude == magn


def test_magnitude_fail_on_zero():
    rn = Rational()
    with pytest.raises(OverflowError):
        m = rn.magnitude


@pytest.mark.parametrize(("num", "den"),
                         ((170, 10),
                          (9 ** 394, 10 ** 247),
                          (-19, 4000)),
                         ids=("compact", "large", "fraction"))
def test_numerator(num, den):
    f = Fraction(num, den)
    rn = Rational(num, den)
    assert rn.numerator == f.numerator


@pytest.mark.parametrize(("num", "den"),
                         ((-17, 1),
                          (9 ** 394, 10 ** 247),
                          (190, 400000)),
                         ids=("compact", "large", "fraction"))
def test_denominator(num, den):
    f = Fraction(num, den)
    rn = Rational(num, den)
    assert rn.denominator == f.denominator


@pytest.mark.parametrize("value",
                         ("17.8",
                          ".".join(("1" * 3297, "4" * 33)),
                          "-0.00014"),
                         ids=("compact", "large", "fraction"))
def test_real_imag(value):
    rn = Rational(value)
    assert rn.real is rn
    assert rn.imag == 0
