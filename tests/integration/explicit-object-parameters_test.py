# SPDX-FileCopyrightText: 2025 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

from itertools import permutations

import explicit_object_parameters as m
import pytest


def test_non_templated_member_function():
    p = m.Plain()
    p.set_double(15)
    assert p.value == 30
    assert p.get_double() == 60


@pytest.mark.parametrize("klass", [m.ExplInstUsing, m.ExplInstInlined])
def test_with_explicit_instantiation(klass):
    obj = klass()
    assert obj.get_value() == 0
    obj.set_value(5)
    assert obj.get_value() == 5


@pytest.mark.parametrize(
    ("klass", "unrelated"), permutations([m.ExplInstUsing, m.ExplInstInlined], 2)
)
def test_ignore_instantiations_with_unrelated_types(klass, unrelated):
    """Ensure that instantiations with unrelated types don't spill into
    derived classes.
    """
    assert unrelated.__name__ not in klass.get_value.__doc__
    assert unrelated.__name__ not in klass.set_value.__doc__


@pytest.mark.parametrize("klass", [m.Using, m.Inlined])
def test_without_explicit_instantiation(klass):
    assert getattr(klass, "get_value", None) is None
    assert getattr(klass, "set_value", None) is None


def test_visible_base():
    # both overloads are exposed on base class
    assert "One" in m.VisibleBase.to_kind.__doc__
    assert "Two" in m.VisibleBase.to_kind.__doc__
    # and can be called on the derived classes
    assert m.One().to_kind() == 1
    assert m.Two().to_kind() == 2
