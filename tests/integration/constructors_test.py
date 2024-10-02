# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import constructors as m
import pytest


def test_can_have_docstring():
    assert "A brief docstring." in m.Example.__init__.__doc__
    assert (
        "Another brief docstring. A default flag value is used."
        in m.Example.__init__.__doc__
    )


def test_can_be_overloaded():
    inst = m.Example(12)
    assert inst.value == 12
    assert inst.flag is False

    inst = m.Example(123, True)
    assert inst.value == 123
    assert inst.flag is True


def test_parameter_names_are_part_of_signature():
    assert ", value: int)" in m.Example.__init__.__doc__
    assert ", value: int, flag: bool)" in m.Example.__init__.__doc__


def test_accept_keyword_arguments():
    inst = m.Example(flag=True, value=42)
    assert inst.flag is True
    assert inst.value == 42

    inst = m.Example(value=42)
    assert inst.flag is False
    assert inst.value == 42


def test_required_arguments_are_supported():
    valid = m.Example(5)
    m.AcceptsNone(valid)
    m.AcceptsNone(None)
    m.RejectsNone(valid)
    with pytest.raises(TypeError, match="incompatible constructor arguments"):
        m.RejectsNone(None)


def test_class_can_be_implicitly_constructible():
    assert m.accepts_implicit(123) == 123
    assert m.accepts_implicit(m.Example(5)) == 5

    assert m.AcceptsImplicit(123).value == 123
    assert m.AcceptsImplicit(m.Example(4)).value == 4

    assert m.accepts_implicit_ref(m.Example(3)) == 3
    assert m.accepts_implicit_ptr(m.Example(2)) == 2
    with pytest.raises(TypeError, match="incompatible function arguments"):
        assert m.accepts_implicit_ptr(None)


def test_implicit_conversion_annotation_can_be_added_to_explicit_constructor():
    assert m.accepts_implicit(m.Onetwothree()) == 123


def test_noconvert_prevents_implicit_conversion():
    with pytest.raises(TypeError, match="incompatible function arguments"):
        m.noconvert_implicit(123)
    assert m.noconvert_implicit(m.Implicit(123)) == 123
    with pytest.raises(TypeError, match="incompatible function arguments"):
        m.noconvert_implicit(m.Example(5))
    assert m.noconvert_implicit(m.Implicit(m.Example(5))) == 5

    with pytest.raises(TypeError, match="incompatible constructor arguments"):
        m.NoconvertImplicit(123)
    assert m.NoconvertImplicit(m.Implicit(123)).value == 123
    with pytest.raises(TypeError, match="incompatible constructor arguments"):
        m.NoconvertImplicit(m.Example(5))
    assert m.NoconvertImplicit(m.Implicit(m.Example(5))).value == 5
