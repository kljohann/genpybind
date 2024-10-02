# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import enums_can_be_arithmetic as m
import pytest


def test_arithmetic_can_use_bitwise_operators():
    state = m.Access.Read | m.Access.Write
    assert isinstance(state, int)
    assert state == 6
    state &= m.Access.Read
    assert isinstance(state, int)
    assert state == m.Access.Read
    assert state == 4


def test_arithmetic_false_cannot_use_bitwise_operators():
    with pytest.raises(TypeError, match="unsupported operand type"):
        m.ExplicitFalse.One | m.ExplicitFalse.Two


def test_arithmetic_true_can_use_bitwise_operators():
    assert m.ExplicitTrue.Four | m.ExplicitTrue.Five == 5


def test_enums_arent_arithmetic_by_default():
    with pytest.raises(TypeError, match="unsupported operand type"):
        m.Default.Seven | m.Default.Eight


def test_arithmetic_enums_support_docstrings():
    assert m.ExplicitTrue.__doc__.startswith("Docstrings are also supported.")
