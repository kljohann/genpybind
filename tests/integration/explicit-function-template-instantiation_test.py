# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import explicit_function_template_instantiation as m
import pytest


def test_non_type_template_argument_single_overload():
    assert m.constant() == 42
    assert "This always returns the same number." in m.constant.__doc__


def test_single_template_argument_overloaded():
    assert m.identity(2.5) == 2.5
    assert m.identity(123) == 123
    assert "Expose instantiations of a function template." in m.identity.__doc__


def test_instantiations_without_arguments_cannot_be_differentiated():
    # TODO: This should be deterministic, if specializations are sorted.
    assert m.oops() in [123, 321]
    assert "not possible" in m.oops.__doc__


def test_instantiations_can_be_renamed():
    assert m.oops_42() == 42


# TODO: Investigate how the comment can be associated with the explicit instantiation.
@pytest.mark.xfail(reason="not available in AST")
def test_renamed_instantiation_has_proper_docstring():
    assert "distinct name" in m.oops_42.__doc__


def test_templated_member_function():
    inst = m.Example()
    assert inst.plus_one(2) == 3
    assert isinstance(inst.plus_one(2), int)
    assert inst.plus_one(2.5) == 3.5


def test_templated_constructor():
    inst = m.TemplatedConstructor(3)
    assert inst.getValue() == 3


def test_templated_member_function_of_template_instance():
    inst = m.Tpl_int_()
    assert inst.magic_value() == 42
