# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import submodules as m


def test_last_name_wins():
    assert m.one.X is m.X
    assert m.deux.X is m.X
    assert not hasattr(m, "two")
    assert m.drei.X is m.X
    assert not hasattr(m, "three")
    assert m.quatre.X is m.X
    assert not hasattr(m, "four")
    assert not hasattr(m, "vier")
    assert m.cinq.X is m.X
    assert not hasattr(m, "five")
    assert not hasattr(m, "fuenf")
    assert m.seis.X is m.X
    assert not hasattr(m, "six")
    assert not hasattr(m, "sechs")


def test_can_be_reopened():
    assert m.reopened.Y()
    assert m.reopened.X is m.X
    assert m.reopened_2.Y()
    assert m.reopened_2.X is m.X
    assert m.reopened_2.Z()
    assert m.reopened_2.X_2 is m.X
