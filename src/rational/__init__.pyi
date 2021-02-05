# coding=utf-8
from __future__ import annotations

import numbers
from abc import abstractmethod
from fractions import Fraction
from typing import Any, Optional, Protocol, Tuple, Type, Union, overload

class SupportsConversionToRational(Protocol):

    @abstractmethod
    def as_integer_ratio(self) -> Tuple[int, int]:
        ...

RationalT = Union[numbers.Rational, SupportsConversionToRational]

class Rational(numbers.Rational):

    @overload
    def __new__(cls: Type[Rational], numerator: None = None,
                denominator: None = None,
                precision: None = None) \
            -> Rational:
        ...
    @overload
    def __new__(cls: Type[Rational], numerator: str = ...,
                denominator: None = None,
                precision: Optional[int] = None) \
            -> Rational:
        ...
    @overload
    def __new__(cls: Type[Rational], numerator: RationalT = ...,
                denominator: Optional[RationalT] = ...,
                precision: Optional[int] = None) \
            -> Rational:
        ...
    @classmethod
    def from_float(cls: Type[Rational], f: Union[float, numbers.Integral]) -> \
            Rational:
        ...
    @classmethod
    def from_decimal(cls: Type[Rational], d: RationalT) -> Rational:
        ...
    @classmethod
    def from_real(cls: Type[Rational], r: numbers.Real, exact: bool = ...) -> \
            Rational:
        ...
    @property
    def numerator(self) -> int:
        ...
    @property
    def denominator(self) -> int:
        ...
    @property
    def real(self) -> Rational:
        ...
    @property
    def imag(self) -> int:
        ...
    def adjusted(self, precision: Optional[int] = ...) -> Rational:
        ...
    def quantize(self, quant: RationalT) -> Rational:
        ...
    def as_fraction(self) -> Fraction:
        ...
    def as_integer_ratio(self) -> Tuple[int, int]:
        ...
    def __copy__(self) -> Rational:
        ...
    def __deepcopy__(self, memo: Any) -> Rational:
        ...
    def __bytes__(self) -> bytes:
        ...
    def __format__(self, fmt_spec: str) -> str:
        ...
    def __eq__(self, other: Any) -> bool:
        ...
    def __lt__(self, other: Any) -> bool:
        ...
    def __le__(self, other: Any) -> bool:
        ...
    def __gt__(self, other: Any) -> bool:
        ...
    def __ge__(self, other: Any) -> bool:
        ...
    def __hash__(self) -> int:
        ...
    def __bool__(self) -> bool:
        ...
    def __int__(self) -> int:
        ...
    def __trunc__(self) -> int:
        ...
    def __complex__(self) -> complex:
        ...
    def __float__(self) -> float:
        ...
    def __pos__(self) -> Rational:
        ...
    def __neg__(self) -> Rational:
        ...
    def __abs__(self) -> Rational:
        ...
    def __add__(self, other: Any) -> Rational:
        ...
    def __radd__(self, other: Any) -> Rational:
        ...
    def __sub__(self, other: Any) -> Rational:
        ...
    def __rsub__(self, other: Any) -> Rational:
        ...
    def __mul__(self, other: Any) -> Rational:
        ...
    def __rmul__(self, other: Any) -> Rational:
        ...
    def __truediv__(self, other: Any) -> Rational:
        ...
    def __rtruediv__(self, other: Any) -> Rational:
        ...
    def __divmod__(self, other: Any) -> Tuple[int, Rational]:
        ...
    def __rdivmod__(self, other: Any) -> Tuple[int, Rational]:
        ...
    def __floordiv__(self, other: Any) -> int:
        ...
    def __rfloordiv__(self, other: Any) -> int:
        ...
    def __mod__(self, other: Any) -> Rational:
        ...
    def __rmod__(self, other: Any) -> Rational:
        ...
    def __pow__(self, other: Any, mod: Any = ...) -> Rational:
        ...
    def __rpow__(self, other: Any, mod: Any = ...) -> Rational:
        ...
    def __floor__(self) -> int:
        ...
    def __ceil__(self) -> int:
        ...
    @overload
    def __round__(self, ndigits: None = ...) -> int:
        ...
    @overload
    def __round__(self, ndigits: int) -> Rational:
        ...