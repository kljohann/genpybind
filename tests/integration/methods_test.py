# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import methods as m
import pytest


def test_can_have_docstring():
    assert "A brief docstring." in m.Example.__doc__
    assert "Another brief docstring." in m.Example.public_method.__doc__


def test_public_methods_are_exposed():
    inst = m.Example()
    assert inst.public_method() == 5
    assert inst.public_const_method() == 123


def test_methods_can_be_renamed():
    inst = m.Example()
    assert not hasattr(inst, "old_name")
    assert inst.new_name() == 42


def test_methods_can_be_overloaded():
    inst = m.Example()
    assert inst.overloaded(5) == 5
    assert isinstance(inst.overloaded(5), int)
    assert inst.overloaded(2.5) == 2.5


def test_methods_can_be_static():
    assert isinstance(m.Example.static_method(), m.Example)


def test_hidden_methods_are_absent():
    inst = m.Example()
    assert not hasattr(inst, "hidden_method")


def test_private_methods_are_absent():
    inst = m.Example()
    assert not hasattr(inst, "private_method")


def test_deleted_methods_are_not_exposed():
    inst = m.Example()
    assert not hasattr(inst, "deleted_method")


def test_using_typedefs_for_member_functions_works():
    assert m.ExoticMemberFunctions().function() == 42
    assert m.ExoticMemberFunctions().other_function() == 123


def test_conversion_functions_are_exposed():
    inst = m.Example()
    assert isinstance(inst.operator_bool(), bool)
    assert hasattr(inst, "__int__")
    assert not hasattr(inst, "operator_int")
    assert int(inst) == 123
    assert isinstance(inst.operator_Other(), m.Other)

    other = m.Other()
    assert not hasattr(inst, "operator_Example")
    assert isinstance(other.toExample(), m.Example)


def test_conversion_function_does_not_imply_implicit_conversion():
    inst = m.Example()
    with pytest.raises(TypeError, match="incompatible constructor arguments"):
        m.Other(inst)

    other = m.Other()
    with pytest.raises(TypeError, match="incompatible constructor arguments"):
        m.Example(other)

    m.consumes_other(other)
    with pytest.raises(TypeError, match="incompatible function arguments"):
        m.consumes_other(inst)


def test_call_operator_is_exposed():
    inst = m.Example()
    assert callable(inst)
    assert inst() == 123
    assert inst(321) == 321


def test_call_operator_can_be_renamed():
    inst = m.Example()
    assert inst.call(0.5) == 0.5
    with pytest.raises(TypeError, match="incompatible function arguments"):
        inst(333.0)
