# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import inspect
import re

import ostream as m


def test_example_has_repr_and_str():
    inst = m.Example(123)
    assert repr(inst) == "Example(123)"
    assert str(inst) == "123"
    assert not hasattr(inst, "__lshift__")


def test_member_functions_are_ignored():
    inst = m.RedHerring()
    assert re.match("<ostream.RedHerring object at .+?>", repr(inst))
    assert str(inst) == repr(inst)
    assert "std::ostream" in inst.__lshift__.__doc__
    assert not inspect.ismethod(inst.__str__)

    # NOTE: `expose_as` is currently ignored for non-ostream operators.
    inst = m.AnotherRedHerring()
    assert "std::ostream" in inst.__lshift__.__doc__
    assert not inspect.ismethod(inst.__str__)


def test_can_be_exposed_as_str():
    inst = m.ExposedAsStr()
    assert str(inst) == "human readable"
    assert re.match("<ostream.ExposedAsStr object at .+?>", repr(inst))
    assert not hasattr(inst, "__lshift__")
    assert "std::ostream" not in inst.__str__.__doc__
    assert inspect.ismethod(inst.__str__)
    assert not inspect.ismethod(inst.__repr__)


def test_can_be_exposed_as_repr():
    inst = m.ExposedAsRepr()
    assert repr(inst) == "ExposedAsRepr()"
    assert str(inst) == repr(inst)
    assert not hasattr(inst, "__lshift__")
    assert "std::ostream" not in inst.__repr__.__doc__
    assert inspect.ismethod(inst.__repr__)
    assert not inspect.ismethod(inst.__str__)
