# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import enumerators_can_have_annotations as m


def test_enumerator_can_be_hidden():
    assert not hasattr(m.Example, "Hidden")


def test_enumerator_can_be_renamed():
    assert not hasattr(m.Example, "Original")
    assert hasattr(m.Example, "Renamed")


def test_enumerators_are_visible_by_default():
    assert hasattr(m.Example, "NotAnnotated")
