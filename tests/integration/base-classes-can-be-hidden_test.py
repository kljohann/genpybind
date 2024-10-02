# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import base_classes_can_be_hidden as m

BASE_CLASSES = frozenset([m.NestedBase, m.Base, m.Base2])


def test_no_arguments_hides_all():
    assert BASE_CLASSES.isdisjoint(m.HideAll.mro())
    derived = m.HideAll()
    assert not isinstance(derived, (m.Base, m.Base2, m.NestedBase))


def test_unqualified_argument():
    derived = m.HideUnqualified()
    assert not isinstance(derived, (m.Base, m.NestedBase))
    assert isinstance(derived, m.Base2)


def test_qualified_argument():
    derived = m.HideQualified()
    assert not isinstance(derived, m.Base)
    assert isinstance(derived, m.NestedBase)
    assert isinstance(derived, m.Base2)


def test_qualified_nested_argument():
    derived = m.HideQualifiedNested()
    assert not isinstance(derived, m.NestedBase)
    assert isinstance(derived, m.Base)
    assert isinstance(derived, m.Base2)


def test_hide_multiple():
    derived = m.HideMultiple()
    assert not isinstance(derived, (m.Base, m.Base2))
    assert isinstance(derived, m.NestedBase)


def test_hide_template():
    derived = m.HideTemplate()
    assert not isinstance(derived, (m.BaseTemplate_int_, m.BaseTemplate_bool_))
    assert isinstance(derived, (m.Base))
