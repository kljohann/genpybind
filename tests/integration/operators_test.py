# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import operator

import operators as m
import pytest
from helpers import get_proper_members

assert (not NotImplemented) is False


def test_only_expected_operators_are_defined():
    assert sorted(get_proper_members(m.Other, callable).keys()) == [
        "__init__",
    ]

    assert sorted(get_proper_members(m.Number, callable).keys()) == [
        "__eq__",
        "__gt__",
        "__init__",
        "__lt__",
    ]

    assert sorted(get_proper_members(m.Integer, callable).keys()) == [
        "__eq__",
        "__gt__",
        "__init__",
        "__lt__",
    ]

    assert sorted(get_proper_members(m.Templated, callable).keys()) == [
        "__eq__",
        "__init__",
    ]


def test_member_function_with_same_type_rhs_by_value():
    inst = m.Number(5)
    assert inst == m.Number(5)
    assert not inst == m.Number(2)


def test_member_function_with_builtin_type_rhs_by_value():
    inst = m.Number(5)
    assert inst > 2
    assert not inst > 123


def test_inline_friend_with_same_type_by_ref():
    inst = m.Number(5)
    assert inst < m.Number(7)
    assert not inst < m.Number(3)


def test_inline_friend_with_builtin_type_by_value():
    inst = m.Number(5)
    assert inst == 5
    assert not inst == 1


def test_reverse_inline_friend_with_custom_type_by_value():
    inst = m.Number(5)
    other = m.Other()
    assert not other == inst
    assert not inst.__eq__(other)
    assert not inst == other
    assert other.__eq__(inst) is NotImplemented


def test_reverse_inline_friend_with_builtin_type_by_value():
    inst = m.Number(5)
    assert 7 > inst
    assert inst.__lt__(7)
    assert not 3 > inst
    assert not inst.__lt__(3)


def test_inline_friend_with_builtin_type():
    inst = m.Number(5)
    assert not inst.__eq__(True)


def test_private_operator_is_not_exposed():
    assert m.Integer(5).__eq__(m.Number(5)) is NotImplemented


def test_in_associated_namespace():
    inst = m.Integer(123)
    assert inst == 123
    assert not inst == 5
    assert inst.__eq__(123)
    assert not inst.__eq__(5)


def test_reverse_in_associated_namespace():
    inst = m.Integer(123)
    assert 5 < inst
    assert inst.__gt__(5)
    assert not 321 < inst
    assert not inst.__gt__(321)


def test_instantiated_template_operators():
    inst = m.Templated(321)
    assert inst == 321
    assert not inst == 123
    assert inst == m.Templated(321)
    assert not inst == m.Templated(123)


BINARY_OPERATORS = (
    "add sub mul truediv mod xor and or lt gt lshift rshift eq ne le ge".split()
)
BINARY_INPLACE_OPERATORS = (
    "iadd isub imul itruediv imod ixor iand ior ilshift irshift".split()
)
UNARY_OPERATORS = "pos neg invert".split()

IS_UNSUPPORTED_RX = r"unsupported operand type\(s\)|not supported between instances of"


def get_operator(name):
    return getattr(operator, f"__{name}__")


@pytest.mark.parametrize("name", BINARY_OPERATORS + BINARY_INPLACE_OPERATORS)
@pytest.mark.parametrize("kind", ["member", "nonconst_member", "friend", "associated"])
def test_exhaustive_has_binary_operator(name, kind):
    cls = getattr(m, f"has_{kind}_{name}")
    assert sorted(get_proper_members(cls, callable).keys()) == sorted(
        ["__init__", f"__{name}__"]
    )
    lhs = cls(123)
    rhs = cls(321)
    for result in [get_operator(name)(lhs, rhs), getattr(lhs, f"__{name}__")(rhs)]:
        assert result.lhs == 123
        assert result.rhs == 321


@pytest.mark.parametrize("name", BINARY_OPERATORS + BINARY_INPLACE_OPERATORS)
@pytest.mark.parametrize(
    "kind", ["hidden_member", "hidden_friend", "hidden_associated"]
)
def test_exhaustive_hidden_binary_operator(name, kind):
    cls = getattr(m, f"has_{kind}_{name}")
    assert sorted(get_proper_members(cls, callable).keys()) == [
        "__init__",
    ]

    if name in ["eq", "ne"]:
        # Python always synthesizes a default function in this case.
        return

    lhs = cls(123)
    rhs = cls(321)
    with pytest.raises(TypeError, match=IS_UNSUPPORTED_RX):
        get_operator(name)(lhs, rhs)


@pytest.mark.parametrize("name", UNARY_OPERATORS)
@pytest.mark.parametrize("kind", ["member", "nonconst_member", "friend", "associated"])
def test_exhaustive_has_unary_operator(name, kind):
    cls = getattr(m, f"has_{kind}_{name}")
    assert sorted(get_proper_members(cls, callable).keys()) == sorted(
        ["__init__", f"__{name}__"]
    )
    oper = get_operator(name)
    inst = cls(123)
    for result in [oper(inst), getattr(inst, f"__{name}__")()]:
        assert result.value == oper(123)


@pytest.mark.parametrize("name", UNARY_OPERATORS)
@pytest.mark.parametrize(
    "kind", ["hidden_member", "hidden_friend", "hidden_associated"]
)
def test_exhaustive_hidden_unary_operator(name, kind):
    cls = getattr(m, f"has_{kind}_{name}")
    assert sorted(get_proper_members(cls, callable).keys()) == [
        "__init__",
    ]

    if name in ["eq", "ne"]:
        # Python always synthesizes a default function in this case.
        return

    inst = cls(123)
    with pytest.raises(TypeError, match="bad operand type for unary"):
        get_operator(name)(inst)
