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

from rational import Rounding


@pytest.fixture(scope="session",
                params=[rnd.name for rnd in Rounding],
                ids=[rnd.name for rnd in Rounding])
def rnd(request) -> Rounding:
    return Rounding[request.param]
