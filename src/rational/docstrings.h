/* ---------------------------------------------------------------------------
Name:        docstrings.h

Author:      Michael Amrhein (michael@adrhinum.de)

Copyright:   (c) 2021 ff. Michael Amrhein
License:     This program is part of a larger application. For license
             details please read the file LICENSE.TXT provided together
             with the application.
------------------------------------------------------------------------------
$Source$
$Revision$
*/

#ifndef RATIONAL_DOCSTRINGS_H
#define RATIONAL_DOCSTRINGS_H

#include <pymacro.h>

// Rational type

PyDoc_STRVAR(
    RationalType_doc,
    "Rational number with a given number of fractional digits.\n\n"
    "Args:\n"
    "   value (see below): numerical value (default: None)\n"
    "   precision (numbers.Integral): number of fractional digits (default:\n"
    "       None)\n\n"
    "If `value` is given, it must either be a string, an instance of\n"
    "`numbers.Integral`, `number.Rational` (for example `fractions.Fraction`),\n"
    "`rational.Rational`, a finite instance of `numbers.Real` (for example "
    "`float`)\n"
    "or be convertable to a `float` or an `int`.\n\n"
    "If a string is given as value, it must be a string in one of two formats:\n\n"
    "    * [+|-]<int>[.<frac>][<e|E>[+|-]<exp>] or\n"
    "    * [+|-].<frac>[<e|E>[+|-]<exp>].\n\n"
    "If given value is `None`, Rational(0) is returned.\n\n"
    "Returns:\n"
    "    :class:`Rational` instance derived from `value` according\n"
    "    to `precision`\n\n"
    "The value is always adjusted to the given precision or the precision is\n"
    "calculated from the given value, if no precision is given.\n\n"
    "Raises:\n"
    "    TypeError: `precision` is given, but not of type `Integral`.\n"
    "    TypeError: `value` is not an instance of the types listed above and\n"
    "        not convertable to `float` or `int`.\n"
    "    ValueError: `precision` is given, but not >= 0.\n"
    "    ValueError: `precision` is given, but not <= `MAX_DEC_PRECISION`.\n"
    "    ValueError: `value` can not be converted to a `Rational` (with a "
    "number\n"
    "        of fractional digits <= `MAX_DEC_PRECISION`).\n\n"
    ":class:`Rational` instances are immutable.\n\n"
);

// Rational class methods

PyDoc_STRVAR(
    RationalType_from_float_doc,
    "Convert a finite float (or int) to a :class:`Rational`.\n\n"
    "Args:\n"
    "    f (float or int): number to be converted to a `Rational`\n\n"
    "Returns:\n"
    "    :class:`Rational` instance derived from `f`\n\n"
    "Raises:\n"
    "    TypeError: `f` is neither a `float` nor an `int`.\n"
    "    ValueError: `f` can not be converted to a :class:`Rational` with\n"
    "        a precision <= the maximal precision.\n\n"
    "Beware that Rational.from_float(0.3) != Rational('0.3').\n"
);

PyDoc_STRVAR(
    RationalType_from_decimal_doc,
    "Convert a finite decimal number to a :class:`Rational`.\n\n"
    "Args:\n"
    "    d (see below): decimal number to be converted to a "
    ":class:`Rational`\n\n"
    "Returns:\n"
    "    :class:`Rational` instance derived from `d`\n\n"
    "Raises:\n"
    "    TypeError: `d` is not an instance of the types listed above.\n"
    "    ValueError: `d` can not be converted to a :class:`Rational`.\n\n"
);

PyDoc_STRVAR(
    Rational_rounded_doc,
    "Return `num` / `den`, rounded to `n_digits` number of fractional digits"
    ".\n\n"
    "Args:\n"
    "   num (numbers.Rational): numerator\n"
    "   den (numbers.Rational): denominator\n"
    "   n_digits (Optional[int]): number of fractional digits (default: 0)\n\n"
    "Returns:\n"
    "    :class:`Rational` instance, equal to `round(Rational(num, den), "
    "        n_digits)\n\n"
    "Raises:\n"
    "    TypeError: `num` or `den` is not a `Rational` and ca not be "
    "        converted to a `Rational`.\n"
    "    TypeError: `n_digits` is given, but not of type `int`.\n"
    "    ValueError: `n_digits` is given, but not >= 0.\n"
    "    ValueError: `n_digits` is given, but not <= `MAX_DEC_PRECISION`.\n"
    "    ValueError: `value` can not be converted to a `Rational` (with a "
    "number\n"
    "        of fractional digits <= `MAX_DEC_PRECISION`).\n\n"
    ":class:`Rational` instances are immutable.\n\n"
);

// Rational properties

PyDoc_STRVAR(
    Rational_precision_doc,
    "Return precision of `self`."
);

PyDoc_STRVAR(
    Rational_magnitude_doc,
    "Return magnitude of `self` in terms of power to 10.\n\n"
    "I.e. the largest integer exp so that 10 ** exp <= self.\n\n"
);

PyDoc_STRVAR(
    Rational_numerator_doc,
    "Return the normalized numerator of `self`.\n\n"
    "I. e. the numerator from the pair of integers with the smallest\n"
    "positive denominator, whose ratio is equal to `self`.\n\n"
);

PyDoc_STRVAR(
    Rational_denominator_doc,
    "Return the normalized denominator of 'self'.\n\n"
    "I. e. the smallest positive denominator from the pairs of integers,\n"
    "whose ratio is equal to `self`.\n\n"
);

PyDoc_STRVAR(
    Rational_real_doc,
    "Return real part of `self`.\n\n"
    "Returns `self` (Real numbers are their real component).\n\n"
);

PyDoc_STRVAR(
    Rational_imag_doc,
    "Return imaginary part of `self`.\n\n"
    "Returns 0 (Real numbers have no imaginary component).\n\n"
);

// Rational instance methods

PyDoc_STRVAR(
    Rational_adjusted_doc,
    "Return adjusted copy of `self`.\n\n"
    "Args:\n"
    "    precision (numbers.Integral): number of fractional digits (default: None)\n"
    "    rounding (ROUNDING): rounding mode (default: None)\n\n"
    "Returns:\n"
    "    :class:`Rational` instance derived from `self`, adjusted to the "
    "given\n"
    "        `precision`, using the given `rounding` mode\n\n"
    "If no `precision` is given, the result is adjusted to the minimum\n"
    "precision preserving x == x.adjusted().\n\n"
    "If no `rounding` mode is given, the current default rounding mode is used.\n\n"
    "If the given `precision` is less than the precision of `self`, the\n"
    "result is rounded and thus information may be lost.\n\n"
);

PyDoc_STRVAR(
    Rational_quantize_doc,
    "Return integer multiple of `quant` closest to `self`.\n\n"
    "Args:\n"
    "    quant (Number): quantum to get a multiple from; must be a `Rational`\n"
    "        or a number which is convertable to a `Rational`\n"
    "        (i. e. must support 'as_integer_ratio')\n"
    "    rounding (ROUNDING): rounding mode (default: None)\n\n"
    "If no `rounding` mode is given, the current default rounding mode is\n"
    "used.\n\n"
    "Returns:\n"
    "    :class:`Rational` instance that is the integer multiple of `quant`\n"
    "        closest to `self` (according to `rounding` mode); if result\n"
    "        can not be represented as :class:`Rational`, an instance of\n"
    "        `Fraction` is returned\n\n"
    "Raises:\n"
    "    TypeError: `quant` is not a number or does not support\n"
    "        `as_integer_ratio`\n"
    "    ValueError: `quant` is not convertable to a `Rational`\n\n"
);

PyDoc_STRVAR(
    Rational_as_fraction_doc,
    "Return an instance of `Fraction` equal to `self`.\n\n"
    "Returns the `Fraction` with the smallest positive denominator, whose\n"
    "ratio is equal to `self`.\n\n");

PyDoc_STRVAR(
    Rational_as_integer_ratio_doc,
    "Return a pair of integers whose ratio is equal to `self`.\n\n"
    "Returns the pair of numerator and denominator with the smallest\n"
    "positive denominator, whose ratio is equal to `self`.\n\n");

// Rational special methods

PyDoc_STRVAR(
    Rational_copy_doc,
    "Return self (Rational instances are immutable).");

PyDoc_STRVAR(
    Rational_getstate_doc,
    "Return state representation of `self`.");

PyDoc_STRVAR(
    Rational_setstate_doc,
    "Set state of `self` from `state`.");

PyDoc_STRVAR(
    Rational_bytes_doc,
    "bytes(self)");

PyDoc_STRVAR(
    Rational_format_doc,
    "Return `self` converted to a string according to `fmt_spec`.\n\n"
    "Args:\n"
    "    fmt_spec (str): a standard format specifier for a number\n\n"
    "Returns:\n"
    "    str: `self` converted to a string according to `fmt_spec`\n\n");

PyDoc_STRVAR(
    Rational_trunc_doc,
    "math.trunc(self)");

PyDoc_STRVAR(
    Rational_floor_doc,
    "math.floor(self)");

PyDoc_STRVAR(
    Rational_ceil_doc,
    "math.ceil(self)");

PyDoc_STRVAR(
    Rational_round_doc,
    "round(self [, n_digits])\n\n"
    "Round `self` to a given precision in decimal digits (default 0).\n"
    "`n_digits` may be negative.\n\n"
    "This method is called by the built-in `round` function. It returns an\n"
    "`int` when called with one argument, otherwise a :class:`Rational`.\n\n");

#endif //RATIONAL_DOCSTRINGS_H
