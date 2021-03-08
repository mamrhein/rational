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

"""Test driver for package 'rational' (adjustments).."""

from decimal import Decimal, getcontext
from fractions import Fraction
import operator
import sys

import pytest
from hypothesis import given, strategies

from rational import (
    Rational, Rounding, get_dflt_rounding_mode, set_dflt_rounding_mode)


ctx = getcontext()
ctx.prec = 3350

RN_MAX_PREC = 9999


@pytest.fixture()
def round_half_up():
    rnd = get_dflt_rounding_mode()
    set_dflt_rounding_mode(Rounding.ROUND_HALF_UP)
    yield
    set_dflt_rounding_mode(rnd)


@pytest.mark.parametrize(("value", "prec", "numerator"),
                         (("17.849", 1, 178),
                          ("17.845", 2, 1785),
                          (".".join(("1" * 3297, "4" * 33)), -3,
                           int("1" * 3294 + "000")),
                          ("2/777", 3, 3)),
                         ids=("compact1", "compact2", "large", "fraction"))
def test_adjust_round_half_up(round_half_up, value, prec, numerator):
    rn = Rational(value)
    adj = rn.adjusted(prec)
    assert adj._prec == prec
    frac = Fraction(numerator, 1 if prec <= 0 else 10 ** prec)
    assert adj.numerator == frac.numerator
    assert adj.denominator == frac.denominator


@given(value=strategies.fractions(),
       prec=strategies.integers(min_value=-RN_MAX_PREC, max_value=RN_MAX_PREC))
def test_adjust_dflt_round_hypo(value, prec):
    rn = Rational(value)
    adj = rn.adjusted(prec)
    assert adj._prec == (prec if value != 0 else 0)
    frac = round(value, prec)
    assert adj.numerator == frac.numerator
    assert adj.denominator == frac.denominator


@pytest.mark.parametrize("value",
                         ("17.849",
                          ".".join(("1" * 3297, "4" * 33)),
                          "0.00015"),
                         ids=("compact", "large", "fraction"))
@pytest.mark.parametrize("prec", (1, -3, 5), ids=("1", "-3", "5"))
def test_adjust_round(rnd, value, prec):
    set_dflt_rounding_mode(rnd)
    rn = Rational(value)
    adj = rn.adjusted(prec)
    assert adj._prec == prec
    quant = Decimal("1e%i" % -prec)
    # compute equivalent Decimal
    eq_dec = Decimal(value).quantize(quant, rnd.name)
    assert adj.as_integer_ratio() == eq_dec.as_integer_ratio()


@given(value=strategies.decimals(allow_nan=False, allow_infinity=False),
       prec=strategies.integers(min_value=-RN_MAX_PREC,
                                max_value=ctx.prec//10))
def test_adjust_round_hypo(rnd, value, prec):
    set_dflt_rounding_mode(rnd)
    rn = Rational(value)
    adj = rn.adjusted(prec)
    assert adj._prec == (prec if value != 0 else 0)
    quant = Decimal("1e%i" % -prec)
    # compute equivalent Decimal
    eq_dec = Decimal(value).quantize(quant, rnd.name)
    assert adj.as_integer_ratio() == eq_dec.as_integer_ratio()


@pytest.mark.parametrize("prec", ["5", 7.5, Fraction(5, 1)],
                         ids=("prec='5'", "prec=7.5", "prec=Fraction(5, 1)"))
def test_adjust_wrong_precision_type(prec):
    rn = Rational('3.12')
    with pytest.raises(TypeError):
        # noinspection PyTypeChecker
        rn.adjusted(precision=prec)


@pytest.mark.parametrize("prec",
                         (RN_MAX_PREC + 1, -RN_MAX_PREC - 1),
                         ids=("max+1", "-max-1"))
def test_adjust_limits_exceeded(prec):
    rn = Rational("4.83")
    with pytest.raises(ValueError):
        rn.adjusted(prec)


@pytest.mark.parametrize("quant", (Fraction(1, 7),
                                   Decimal("-0.3"),
                                   0.4,
                                   3,
                                   1),
                         ids=("1/7",
                              "Decimal -0.3",
                              "0.4",
                              "3",
                              "1"))
@pytest.mark.parametrize("value",
                         ("17.849",
                          ".".join(("1" * 2259, "4" * 33)),
                          "0.0025",
                          "12345678901234567e12"),
                         ids=("compact", "large", "fraction", "int"))
def test_quantize_dflt_round(value, quant):
    set_dflt_rounding_mode(Rounding.ROUND_HALF_EVEN)
    rn = Rational(value)
    adj = rn.quantize(quant)
    # compute equivalent Fraction
    quot = Fraction(quant)
    equiv = round(Fraction(value) / quot) * quot
    assert adj.as_integer_ratio() == equiv.as_integer_ratio()


@pytest.mark.parametrize("quant", (Fraction(1, 40),
                                   Decimal("-0.3"),
                                   0.4,
                                   3,
                                   1),
                         ids=("1/40",
                              "Decimal -0.3",
                              "0.4",
                              "3",
                              "1"))
@pytest.mark.parametrize("value",
                         ("17.849",
                          ".".join(("1" * 2259, "4" * 33)),
                          "0.0000000025",
                          "12345678901234567e12"),
                         ids=("compact",
                              "large",
                              "fraction",
                              "int"))
def test_quantize_round(rnd, value, quant):
    set_dflt_rounding_mode(rnd)
    rn = Rational(value)
    adj = rn.quantize(quant)
    # compute equivalent Decimal
    if isinstance(quant, Fraction):
        q = Decimal(quant.numerator) / Decimal(quant.denominator)
    else:
        q = Decimal(quant)
    eq_dec = (Decimal(value) / q).quantize(1, rnd.name) * q
    assert adj.as_integer_ratio() == eq_dec.as_integer_ratio()


@given(value=strategies.decimals(allow_nan=False, allow_infinity=False),
       quant=strategies.decimals(allow_nan=False, allow_infinity=False,
                                 places=3).filter(lambda x: x != 0))
def test_quantize_decimal_hypo(rnd, value, quant):
    set_dflt_rounding_mode(rnd)
    rn = Rational(value)
    adj = rn.quantize(quant)
    assert get_dflt_rounding_mode() == rnd
    # compute equivalent Decimal
    eq_dec = (value / quant).quantize(1, rnd.name) * quant
    assert adj.as_integer_ratio() == eq_dec.as_integer_ratio()


@pytest.mark.parametrize("quant", (Fraction(1, 3), Fraction(5, 7),
                                   Rational(7, 4)),
                         ids=("Fraction 1/3", "Fraction 5/7", "Rational 7/4"))
@pytest.mark.parametrize("value",
                         ("17.5", "15"),
                         ids=("17.5", "15"))
def test_quantize_to_non_decimal(value, quant):
    set_dflt_rounding_mode(Rounding.ROUND_HALF_EVEN)
    rn = Rational(value)
    adj = rn.quantize(quant)
    # compute equivalent Fraction
    quant = Fraction(quant.numerator, quant.denominator)
    equiv = round(Fraction(value) / quant) * quant
    assert adj.numerator == equiv.numerator
    assert adj.denominator == equiv.denominator


@given(value=strategies.fractions(),
       quant=strategies.fractions().filter(lambda x: x != 0))
def test_quantize_frac_hypo(value, quant):
    set_dflt_rounding_mode(Rounding.ROUND_HALF_EVEN)
    rn = Rational(value)
    adj = rn.quantize(quant)
    # compute equivalent Fraction
    equiv = round(value / quant) * quant
    assert adj.numerator == equiv.numerator
    assert adj.denominator == equiv.denominator


@pytest.mark.parametrize("quant", ["0.5", 7.5 + 3j, Fraction],
                         ids=("quant='0.5'", "quant=7.5+3j",
                              "quant=Fraction"))
def test_quantize_wrong_quant_type(quant):
    rn = Rational('3.12')
    with pytest.raises(TypeError):
        rn.quantize(quant)


@pytest.mark.parametrize("quant", [float('inf'), Decimal('-inf'),
                                   float('nan'), Decimal('NaN'),
                                   0, Decimal(0), Fraction(0, 1)],
                         ids=("quant='inf'", "quant='-inf'",
                              "quant='nan'", "quant='NaN'",
                              "quant=0", "quant=Decimal(0)",
                              "quant=Fraction(0,1)"))
def test_quantize_incompat_quant_value(quant):
    rn = Rational('3.12')
    with pytest.raises(ValueError):
        rn.quantize(quant)


@pytest.mark.parametrize("value",
                         ("-17.849",
                          ".".join(("1" * 3297, "4" * 33)),
                          "0.00015",
                          Fraction(16, 3)),
                         ids=("compact", "large", "less1", "fraction"))
@pytest.mark.parametrize("prec", (0, -1, -3, 4), ids=lambda p: str(p))
def test_round_to_prec(value, prec):
    rn = Rational(value)
    adj = round(rn, prec)
    assert isinstance(adj, Rational)
    assert adj._prec == (prec if value != 0 else 0)
    assert adj.as_fraction() == round(rn.as_fraction(), prec)


@given(value=strategies.fractions(),
       prec=strategies.integers(min_value=-RN_MAX_PREC, max_value=RN_MAX_PREC))
def test_round_to_prec_hypo(value, prec):
    rn = Rational(value)
    adj = round(rn, prec)
    assert isinstance(adj, Rational)
    assert adj._prec == (prec if value != 0 else 0)
    assert adj.as_fraction() == round(value, prec)


@pytest.mark.parametrize("value",
                         ("-17.849",
                          ".".join(("1" * 3297, "4" * 33)),
                          "0.00015",
                          "999999999999999999.67",),
                         ids=("compact", "large", "fraction", "carry",))
def test_round_to_int(value):
    rn = Rational(value)
    adj = round(rn)
    assert isinstance(adj, int)
    assert adj == round(Fraction(value))


@given(value=strategies.fractions())
def test_round_to_int_hypo(value):
    rn = Rational(value)
    adj = round(rn)
    assert isinstance(adj, int)
    assert adj == round(value)


@pytest.mark.parametrize("value",
                         ("17.8",
                          ".".join(("1" * 3297, "4" * 33)),
                          "-0.00014"),
                         ids=("compact", "large", "fraction"))
def test_pos(value):
    rn = Rational(value)
    assert +rn is rn


@pytest.mark.parametrize("value",
                         ("17.8",
                          "-" + ".".join(("1" * 3297, "4" * 33)),
                          "-0.00014",
                          "0.00"),
                         ids=("compact", "large", "fraction", "0"))
def test_neg(value):
    rn = Rational(value)
    assert -(-rn) == rn
    assert -(-(-rn)) == -rn
    if rn <= 0:
        assert -rn >= 0
    else:
        assert -rn <= 0


@pytest.mark.parametrize("value",
                         ("17.8",
                          "-" + ".".join(("1" * 3297, "4" * 33)),
                          "-0.00014",
                          "0.00"),
                         ids=("compact", "large", "fraction", "0"))
def test_abs(value):
    rn = Rational(value)
    assert abs(rn) >= 0
    if rn < 0:
        assert abs(rn) == -rn
    else:
        assert abs(rn) == rn
