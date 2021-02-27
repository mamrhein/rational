# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------------
# Copyright:   (c) 2021 ff. Michael Amrhein (michael@adrhinum.de)
# License:     This program is part of a larger application. For license
#              details please read the file LICENSE.TXT provided together
#              with the application.
# ----------------------------------------------------------------------------
# $Source$
# $Revision$

"""Test driver for package 'rational' (comparisons)."""

from decimal import Decimal
from decimal import getcontext, InvalidOperation
from fractions import Fraction
import operator
import sys

import pytest

from rational import Rational


EQUALITY_OPS = (operator.eq, operator.ne)
ORDERING_OPS = (operator.le, operator.lt, operator.ge, operator.gt)
CMP_OPS = EQUALITY_OPS + ORDERING_OPS

ctx = getcontext()
ctx.prec = 3350


@pytest.mark.parametrize("y",
                         ("17.800",
                          ".".join(("1" * 2259, "4" * 33 + "0" * 19)),
                          "-14/33333",
                          "-0"),
                         ids=("compact", "large", "fraction", "zero"))
@pytest.mark.parametrize("x",
                         ("17.800",
                          ".".join(("1" * 2259, "4" * 33 + "0" * 19)),
                          "-14/33333",
                          "0"),
                         ids=("compact", "large", "fraction", "zero"))
@pytest.mark.parametrize("op",
                         [op for op in CMP_OPS],
                         ids=[op.__name__ for op in CMP_OPS])
def test_cmp(op, x, y):
    x1 = Rational(x)
    x2 = Fraction(x)
    y1 = Rational(y)
    y2 = Fraction(y)
    assert op(x1, y1) == op(x2, y2)
    assert op(y1, x1) == op(y2, x2)
    assert op(-x1, y1) == op(-x2, y2)
    assert op(-y1, x1) == op(-y2, x2)
    assert op(x1, -y1) == op(x2, -y2)
    assert op(y1, -x1) == op(y2, -x2)


# noinspection PyMissingOrEmptyDocstring
def chk_eq(dec, equiv):
    assert dec == equiv
    assert equiv == dec
    assert dec >= equiv
    assert equiv <= dec
    assert dec <= equiv
    assert equiv >= dec
    assert not(dec != equiv)
    assert not(equiv != dec)
    assert not(dec > equiv)
    assert not(equiv < dec)
    assert not(dec < equiv)
    assert not(equiv > dec)
    # x == y  <=> hash(x) == hash (y)
    assert hash(dec) == hash(equiv)


@pytest.mark.parametrize("trail", (".000", ".", ""),
                         ids=("trail='000'", "trail='.'", "trail=''"))
@pytest.mark.parametrize("value",
                         ("-17",
                          "".join(("1" * 3097, "4" * 33, "0" * 19)),
                          "-0"),
                         ids=("compact", "large", "zero"))
def test_eq_integral(value, trail):
    rn = Rational(value + trail)
    equiv = int(value)
    chk_eq(rn, equiv)


@pytest.mark.parametrize("trail2", ("0000", ""),
                         ids=("trail2='0000'", "trail2=''"))
@pytest.mark.parametrize("trail1", ("000", ""),
                         ids=("trail1='000'", "trail1=''"))
@pytest.mark.parametrize("value",
                         ("17.800",
                          ".".join(("1" * 3097, "4" * 33 + "0" * 19)),
                          "-0.00014"),
                         ids=("compact", "large", "fraction"))
@pytest.mark.parametrize("rational",
                         (None, Decimal, Fraction),
                         ids=("Rational", "Decimal", "Fraction"))
def test_eq_rational(rational, value, trail1, trail2):
    if rational is None:
        rational = Rational
    rn = Rational(value + trail1)
    equiv = rational(value + trail2)
    chk_eq(rn, equiv)


@pytest.mark.parametrize("value",
                         ("17.500",
                          sys.float_info.max * 0.9,
                          "%1.63f" % (1 / sys.maxsize)),
                         ids=("compact", "large", "fraction"))
def test_eq_real(value):
    rn = Rational(value)
    equiv = float(value)
    chk_eq(rn, equiv)


@pytest.mark.parametrize("value",
                         ("17.500",
                          sys.float_info.max,
                          "%1.63f" % (1 / sys.maxsize)),
                         ids=("compact", "large", "fraction"))
def test_eq_complex(value):
    rn = Rational(value)
    equiv = complex(value)
    non_equiv = complex(float(value), 1)
    assert rn == equiv
    assert equiv == rn
    assert not (rn != equiv)
    assert not(equiv != rn)
    assert rn != non_equiv
    assert non_equiv != rn
    assert not (rn == non_equiv)
    assert not(non_equiv == rn)


# noinspection PyMissingOrEmptyDocstring
def chk_gt(rn, non_equiv_gt):
    assert rn != non_equiv_gt
    assert non_equiv_gt != rn
    assert rn < non_equiv_gt
    assert non_equiv_gt > rn
    assert rn <= non_equiv_gt
    assert non_equiv_gt >= rn
    assert not(rn == non_equiv_gt)
    assert not(non_equiv_gt == rn)
    assert not(rn > non_equiv_gt)
    assert not(non_equiv_gt < rn)
    assert not(rn >= non_equiv_gt)
    assert not(non_equiv_gt <= rn)
    # x != y  <=> hash(x) != hash (y)
    assert hash(rn) != hash(non_equiv_gt)


# noinspection PyMissingOrEmptyDocstring
def chk_lt(rn, non_equiv_lt):
    assert rn != non_equiv_lt
    assert non_equiv_lt != rn
    assert rn > non_equiv_lt
    assert non_equiv_lt < rn
    assert rn >= non_equiv_lt
    assert non_equiv_lt <= rn
    assert not(rn == non_equiv_lt)
    assert not(non_equiv_lt == rn)
    assert not(rn < non_equiv_lt)
    assert not(non_equiv_lt > rn)
    assert not(rn <= non_equiv_lt)
    assert not(non_equiv_lt >= rn)
    # x != y  <=> hash(x) != hash (y)
    assert hash(rn) != hash(non_equiv_lt)


@pytest.mark.parametrize("value",
                         ("-17",
                          "".join(("1" * 759, "4" * 33, "0" * 19)),
                          "-0"),
                         ids=("compact", "large", "zero"))
def test_ne_integral(value):
    non_equiv = int(value)
    prec = len(value)
    delta = Fraction(1, 10 ** prec)
    rn_gt = Rational(non_equiv + delta)
    chk_lt(rn_gt, non_equiv)
    rn_lt = Rational(non_equiv - delta)
    chk_gt(rn_lt, non_equiv)


@pytest.mark.parametrize("value",
                         ("17.800",
                          ".".join(("1" * 3097, "4" * 33 + "0" * 19)),
                          "-0.00014"),
                         ids=("compact", "large", "fraction"))
@pytest.mark.parametrize("rational",
                         (Decimal, Fraction),
                         ids=("Decimal", "Fraction"))
def test_ne_rational(rational, value):
    rn = Rational(value)
    non_equiv_gt = rational(value) + rational(1) / 10 ** 180
    chk_gt(rn, non_equiv_gt)
    non_equiv_lt = rational(value) - rational(1) / 10 ** 180
    chk_lt(rn, non_equiv_lt)


@pytest.mark.parametrize("value",
                         ("17.500",
                          sys.float_info.max * 0.9,
                          "%1.63f" % (1 / sys.maxsize)),
                         ids=("compact", "large", "fraction"))
def test_ne_real(value):
    rn = Rational(value)
    non_equiv_gt = float(value) * (1. + 1. / 10 ** 14)
    chk_gt(rn, non_equiv_gt)
    non_equiv_lt = float(value) * (1. - 1. / 10 ** 14)
    chk_lt(rn, non_equiv_lt)


@pytest.mark.parametrize("value", ["0.4", "1.75+3j"],
                         ids=("other='0.4'", "1.75+3j"))
@pytest.mark.parametrize("op",
                         [op for op in ORDERING_OPS],
                         ids=[op.__name__ for op in ORDERING_OPS])
def test_ord_ops_complex(op, value):
    rn = Rational('3.12')
    cmplx = complex(value)
    with pytest.raises(TypeError):
        op(rn, cmplx)
    with pytest.raises(TypeError):
        op(cmplx, rn)


@pytest.mark.parametrize("other", ["1/5", operator.ne],
                         ids=("other='1/5'", "other=operator.ne"))
def test_eq_ops_non_number(other):
    rn = Rational('3.12')
    assert not(rn == other)
    assert not(other == rn)
    assert rn != other
    assert other != rn


@pytest.mark.parametrize("other", ["1/5", operator.ne],
                         ids=("other='1/5'", "other=operator.ne"))
@pytest.mark.parametrize("op",
                         [op for op in ORDERING_OPS],
                         ids=[op.__name__ for op in ORDERING_OPS])
def test_ord_ops_non_number(op, other):
    rn = Rational('3.12')
    with pytest.raises(TypeError):
        op(rn, other)
    with pytest.raises(TypeError):
        op(other, rn)


@pytest.mark.parametrize("other", [float('Inf'), Decimal('Inf')],
                         ids=("other='inf' (float)", "other='inf' (Decimal)"))
def test_inf(other):
    rn = Rational()
    chk_gt(rn, other)


@pytest.mark.parametrize("other", [float('Nan'), Decimal('Nan')],
                         ids=("other='nan' (float)", "other='nan' (Decimal)"))
def test_eq_ops_nan(other):
    rn = Rational()
    assert not(rn == other)
    assert not(other == rn)
    assert rn != other
    assert other != rn


@pytest.mark.parametrize("op",
                         [op for op in ORDERING_OPS],
                         ids=[op.__name__ for op in ORDERING_OPS])
def test_ord_ops_float_nan(op):
    rn = Rational()
    assert not op(rn, float('Nan'))
    assert not op(float('Nan'), rn)


@pytest.mark.parametrize("op",
                         [op for op in ORDERING_OPS],
                         ids=[op.__name__ for op in ORDERING_OPS])
def test_ord_ops_decimal_nan(op):
    rn = Rational()
    with pytest.raises(InvalidOperation):
        op(rn, Decimal('Nan'))
    with pytest.raises(InvalidOperation):
        op(Decimal('Nan'), rn)
