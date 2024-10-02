# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import namespaces_can_be_reopened as m


def test_visible_functions_are_exposed():
    assert m.visible_in_first()
    assert m.visible_in_second()


def test_hidden_functions_are_absent():
    assert not hasattr(m, "hidden_in_first")
    assert not hasattr(m, "hidden_in_second")


def test_submodule_can_be_reopened():
    assert m.submodule.visible_in_first()
    assert m.submodule.visible_in_second()


def test_original_namespace_can_be_empty():
    assert m.ButOnlyLater()
    assert m.and_later()
