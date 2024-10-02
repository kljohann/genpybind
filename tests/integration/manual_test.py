# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import manual as m


def test_manual_method():
    inst = m.Example()
    assert inst.manual_method()


def test_manual_method_with_lambda():
    inst = m.Example()
    assert not inst[0]
    assert not inst[1]
    inst[1] = True
    assert inst[1]
    inst[0] = True
    assert inst[0]


def test_manual_class():
    m.Hidden()
