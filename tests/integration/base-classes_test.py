# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import base_classes as m
from helpers import get_user_mro


def test_classes_are_present():
    for name in [
        "Base",
        "CRTP_DerivedCRTP_",
        "Derived",
        "DerivedCRTP",
        "DerivedFromHidden",
        "DerivedPrivate",
        "DerivedProtected",
    ]:
        assert hasattr(m, name)
    assert not hasattr(m, "Hidden")


def test_mro():
    assert get_user_mro(m.Derived) == [m.Derived, m.Base]
    assert get_user_mro(m.DerivedCRTP) == [m.DerivedCRTP, m.CRTP_DerivedCRTP_]
    assert get_user_mro(m.DerivedPrivate) == [m.DerivedPrivate]
    assert get_user_mro(m.DerivedProtected) == [m.DerivedProtected]


def test_isinstance():
    derived = m.Derived()
    assert isinstance(derived, m.Derived)
    assert isinstance(derived, m.Base)
    assert not isinstance(derived, m.DerivedCRTP)
    assert not isinstance(derived, m.CRTP_DerivedCRTP_)

    derived_crtp = m.DerivedCRTP()
    assert not isinstance(derived_crtp, m.Derived)
    assert not isinstance(derived_crtp, m.Base)
    assert isinstance(derived_crtp, m.DerivedCRTP)
    assert isinstance(derived_crtp, m.CRTP_DerivedCRTP_)

    derived_private = m.DerivedPrivate()
    assert not isinstance(derived_private, m.Base)
    assert isinstance(derived_private, m.DerivedPrivate)

    derived_protected = m.DerivedProtected()
    assert not isinstance(derived_protected, m.Base)
    assert isinstance(derived_protected, m.DerivedProtected)


def test_can_be_derived_from_abstract_base():
    inst = m.DerivedFromAbstract()
    assert isinstance(inst, m.Abstract)
    assert inst.abstract() == 7
    assert inst.defined_in_base() == 5
    assert inst.overridden() is True
    assert m.Abstract.static_method() == m.DerivedFromAbstract.static_method()
    assert m.Abstract.static_method() is True
