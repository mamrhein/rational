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


"""Shared pytest fixtures.."""

import pytest

from rational import Rounding, get_dflt_rounding_mode, set_dflt_rounding_mode


@pytest.fixture(scope="session",
                params=[rnd.name for rnd in Rounding],
                ids=[rnd.name for rnd in Rounding])
def rnd(request) -> Rounding:
    return Rounding[request.param]


def dflt_round(rnd):
    @pytest.fixture()
    def closure():
        prev_rnd = get_dflt_rounding_mode()
        set_dflt_rounding_mode(rnd)
        yield
        set_dflt_rounding_mode(prev_rnd)
    return closure


with_round_half_up = dflt_round(Rounding.ROUND_HALF_UP)
with_round_half_even = dflt_round(Rounding.ROUND_HALF_EVEN)
