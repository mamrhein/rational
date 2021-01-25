The package _rational_ provides a _Rational_ number type which can represent
rational numbers of (nearly) arbitrary magnitude and precision. It can be used 
as a replacement for _fractions.Fraction_, with some addtional functions and, 
up to some magnitude, a better performance.

### Usage

_rational.Rational_ instances are created by giving a _numerator_ (default: 0) 
and a _denominator_ (default: 1).

If _numerator_ is given, it must either be a string, an _instance of 
_number.Rational_, or it must support a conversion to a rational number via a 
method _as_integer_ratio_.

If a string is given as _numerator_, it must be a string in one of the 
following formats:

  *  [+|-]\<int>/\<int>
  *  [+|-]\<int>[.\<frac>][<e|E>[+|-]\<exp>]
  *  [+|-].\<frac>[<e|E>[+|-]\<exp>].

If a _denominator_ is given, it must either be an _instance of 
_number.Rational_ or support _as_integer_ratio_, and, in this case, the 
_numerator_ must not be a string.

Optionally, the resulting value can be constraint by giving a _precision_ and 
a _rounding_ mode. _precision_ denotes the maximum number of fractional 
decimal digits for the resulting value, _rounding_ specifies the rounding
mode applied if necessary.

If _precision_ is given, it must be of type _int_.

When the given _precision_ is lower than the precision of the quotient 
_numerator_ / _denominator_, the result is rounded according to the given 
rounding mode, or, if None is given, the current default rounding mode (which 
itself defaults to ROUND_HALF_EVEN).

#### Computations

When importing _rational_, its _Rational_ type is registered in Pythons
numerical stack as subclass of _number.Rational_. It supports all operations 
defined for that base class and its instances can be mixed in computations 
with instances of all numeric types mentioned above.

But, _Rational_ does not deal with infinity, division by 0 always raises a
_ZeroDivisionError_. Likewise, infinite instances of type _float_ or
_decimal.Decimal_ can not be converted to _Rational_ instances. The same is
true for the 'not a number' instances of these types.

All numerical operations give an exact result, i.e. they are not automatically
constraint to the precision of the operands or to a number of significant
digits (like the floating-point _Decimal_ type from the standard module
_decimal_).

_Rational_ supports rounding via the built-in function _round_ using the same
rounding mode as the _float_ type by default (ROUND_HALF_EVEN). In addition, 
via the method _adjusted_, a _Rational_ with a different precision can be 
derived, supporting rounding modes equivalent to those defined by the standard 
library module _decimal_, via the enumeration _ROUNDING_.

For more details see the documentation provided with the source distribution
or [here](https://rational.readthedocs.io/en/latest).
