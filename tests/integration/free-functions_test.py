# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import free_functions as m


def test_has_correct_docstring():
    assert "Inverts the specified `value`." in m.invert.__doc__
    assert m.invert(False)
    assert not m.invert(True)


def test_can_be_overloaded():
    assert m.add(1, 2) == 3
    assert isinstance(m.add(1, 2), int)
    assert m.add(1.0, 2.5) == 3.5


def test_can_be_exposed_using_different_name():
    assert not hasattr(m, "old_name")
    assert m.new_name() == 123


def test_can_be_nested_in_nonsubmodule_namespace():
    assert m.visible()
    assert not hasattr(m, "example")


def test_can_be_hidden_or_not_exposed():
    assert not hasattr(m, "hidden")
    assert not hasattr(m, "not_exposed")


def test_deleted_functions_are_not_exposed():
    assert not hasattr(m, "deleted")
