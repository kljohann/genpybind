# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import re

import args_and_kwargs as m
import pytest


def test_accepts_arbitrary_number_of_arguments():
    assert m.number_of_arguments(1, 2, 3, a=4, b=5, c=6) == 6


def test_mixed_arguments():
    assert m.mixed(10, 1, 2, 3, a=4, b=5, c=6) == 16
    assert m.mixed(a=4, b=5, c=6, offset=10) == 13


def test_trailing_arguments():
    with pytest.raises(
        TypeError, match=re.escape("(offset: int, *args, multiplier: int) -> int")
    ):
        assert m.trailing_arguments(10, 1, 2, 2) == 24
    assert m.trailing_arguments(10, 1, 2, multiplier=2) == 24


def test_by_value():
    assert m.by_value(1, 2, 3, a=4, b=5, c=6) == 6
