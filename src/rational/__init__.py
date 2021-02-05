# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------------
# Copyright:   (c) 2021 ff. Michael Amrhein (michael@adrhinum.de)
# License:     This program is part of a larger application. For license
#              details please read the file LICENSE.TXT provided together
#              with the application.
# ----------------------------------------------------------------------------
# $Source$
# $Revision$


"""Rational number arithmetic."""

from .rational import Rational
from .rounding import Rounding, get_dflt_rounding_mode, set_dflt_rounding_mode
from .version import version_tuple as __version__  # noqa: F401

# define public namespace
__all__ = [
    'Rational',
    'Rounding',
    'get_dflt_rounding_mode',
    'set_dflt_rounding_mode',
]
