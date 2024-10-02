# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import properties as m
import pytest


def test_regular_property():
    inst = m.Example()
    assert not hasattr(inst, "getValue")
    assert not hasattr(inst, "setValue")
    assert inst.value == 0
    inst.value = 5
    assert inst.value == 5
    assert inst.value_ == 5


def test_readonly_property():
    inst = m.Example()
    assert not hasattr(inst, "getReadonly")
    assert inst.readonly is True
    with pytest.raises(AttributeError, match="can't set attribute|has no setter"):
        inst.readonly = False


@pytest.mark.xfail(reason="default argument cannot be propagated to pybind11")
def test_same_method_for_both():
    inst = m.Example()
    assert not hasattr(inst, "strange")
    assert inst.combined == 0
    inst.combined = 4
    assert inst.combined == 4
    assert inst.combined_ == 4


def test_multiple_names():
    inst = m.Example()
    assert not hasattr(inst, "getMulti")
    assert not hasattr(inst, "setMulti")
    assert inst.multi_1 == 0
    assert inst.multi_2 == 0
    inst.multi_1 = 1
    assert inst.multi_1 == 1
    assert inst.multi_2 == 1
    assert inst.multi_ == 1
    inst.multi_2 = 2
    assert inst.multi_1 == 2
    assert inst.multi_2 == 2
    assert inst.multi_ == 2
    inst.multi_ = 3
    assert inst.multi_1 == 3
    assert inst.multi_2 == 3
