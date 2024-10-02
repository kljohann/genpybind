# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import fields_and_variables as m
import pytest


def test_global_variables_are_exposed():
    assert m.global_variable == 123
    assert m.global_const_variable == 456


def test_static_variables_are_exposed():
    assert m.Example.static_variable == 1
    assert m.Example.static_const_variable == 2
    assert m.Example.static_constexpr_variable == 3
    assert m.Example.readonly_static_variable == 1
    assert m.Example.readonly_static_const_variable == 2


def test_static_variables_can_be_writable():
    assert m.Example.static_variable == 1
    m.Example.static_variable = 2
    assert m.Example.static_variable == 2
    m.Example.static_variable = 1
    assert m.Example.static_variable == 1


def test_static_variables_can_be_readonly():
    with pytest.raises(AttributeError, match="can't set attribute|has no setter"):
        m.Example.static_const_variable = 2
    with pytest.raises(AttributeError, match="can't set attribute|has no setter"):
        m.Example.static_constexpr_variable = 3
    with pytest.raises(AttributeError, match="can't set attribute|has no setter"):
        m.Example.readonly_static_variable = 1
    with pytest.raises(AttributeError, match="can't set attribute|has no setter"):
        m.Example.readonly_static_const_variable = 2


def test_fields_can_be_writable():
    inst = m.Example()
    assert inst.field == 1
    inst.field = 2
    assert inst.field == 2
    inst.field = 1
    assert inst.field == 1


def test_fields_can_be_readonly():
    inst = m.Example()
    with pytest.raises(AttributeError, match="can't set attribute|has no setter"):
        inst.const_field = 2
    with pytest.raises(AttributeError, match="can't set attribute|has no setter"):
        inst.readonly_field = 3
    with pytest.raises(AttributeError, match="can't set attribute|has no setter"):
        inst.readonly_const_field = 4


def test_fields_are_exposed():
    inst = m.Example()
    assert inst.field == 1
    assert inst.const_field == 2
    assert inst.readonly_field == 3
    assert inst.readonly_const_field == 4
