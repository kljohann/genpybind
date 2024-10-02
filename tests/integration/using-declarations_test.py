# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import pytest
import using_declarations as m


@pytest.mark.xfail(reason="bug: uses address of base variable")
def test_can_expose_protected_member_of_base():
    m.Derived().m_protected = -123
    m.DerivedHidden().m_protected = -123
    m.DerivedInlined().m_protected = -123
    m.DerivedAnnotated().protected = -123


def test_can_expose_member_of_hidden_base():
    m.DerivedHidden().m_public = 123


def test_can_hide_shadowed_member_of_inlined_base():
    assert not hasattr(m.DerivedInlined(), "m_public")


@pytest.mark.xfail(reason="not implemented, annotations are ignored")
def test_can_rename_member():
    m.DerivedAnnotated().public = 123
    m.DerivedAnnotated().protected = -123
    assert not hasattr(m.DerivedAnnotated(), "m_public")
    assert not hasattr(m.DerivedAnnotated(), "m_protected")


@pytest.mark.xfail(reason="bug: also pulls in implicit copy ctor")
def test_can_pull_in_constructor_from_base():
    assert m.Ctor(456).value == 456


def test_can_pull_in_function_template_from_base():
    assert m.Tpl().neg(42) == -42
