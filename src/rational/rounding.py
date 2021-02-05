# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------------
# Copyright:   (c) 2018 ff. Michael Amrhein (michael@adrhinum.de)
# License:     This program is part of a larger application. For license
#              details please read the file LICENSE.TXT provided together
#              with the application.
# ----------------------------------------------------------------------------
# $Source$
# $Revision$

"""Rounding modes for rational number arithmetic."""

from __future__ import annotations

from contextvars import ContextVar, Token
from enum import Enum, unique


__all__ = ['Rounding', 'get_dflt_rounding_mode', 'set_dflt_rounding_mode']


# rounding modes equivalent to those defined in standard lib module 'decimal'
@unique
class Rounding(Enum):
    """Enumeration of rounding modes."""

    def __new__(cls, value: int, doc: str) -> Rounding:
        """Return new member of the Enum."""
        member = object.__new__(cls)
        member._value_ = value
        member.__doc__ = doc
        return member

    ROUND_05UP = (1, 'Round away from zero if last digit after rounding '
                     'towards zero would have been 0 or 5; otherwise round '
                     'towards zero.')
    ROUND_CEILING = (2, 'Round towards Infinity.')
    ROUND_DOWN = (3, 'Round towards zero.')
    ROUND_FLOOR = (4, 'Round towards -Infinity.')
    ROUND_HALF_DOWN = (5, 'Round to nearest with ties going towards zero.')
    ROUND_HALF_EVEN = (6, 'Round to nearest with ties going to nearest even '
                          'integer.')
    ROUND_HALF_UP = (7, 'Round to nearest with ties going away from zero.')
    ROUND_UP = (8, 'Round away from zero.')


_dflt_rounding: ContextVar[Rounding] = \
    ContextVar("dflt_rounding", default=Rounding.ROUND_HALF_EVEN)


def get_dflt_rounding_mode() -> Rounding:
    """Return default rounding mode."""
    return _dflt_rounding.get()


def set_dflt_rounding_mode(rounding: Rounding) -> Token:
    """Set default rounding mode.

    Args:
        rounding (ROUNDING): rounding mode to be set as default

    Raises:
        TypeError: given 'rounding' is not a valid rounding mode
    """
    if not isinstance(rounding, Rounding):
        raise TypeError(f"Illegal rounding mode: {rounding!r}")
    return _dflt_rounding.set(rounding)
