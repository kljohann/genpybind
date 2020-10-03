import base_classes as m


def test_classes_are_present():
    for name in [
        "Base",
        "CRTP_DerivedCRTP_",
        "Derived",
        "DerivedCRTP",
        "DerivedFromHidden",
    ]:
        assert hasattr(m, name)
    assert not hasattr(m, "Hidden")


def test_mro():
    assert m.Derived.mro()[:2] == [m.Derived, m.Base]
    assert m.DerivedCRTP.mro()[:2] == [m.DerivedCRTP, m.CRTP_DerivedCRTP_]


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


def test_can_be_derived_from_abstract_base():
    inst = m.DerivedFromAbstract()
    assert isinstance(inst, m.Abstract)
    assert inst.abstract() == 7
    assert inst.defined_in_base() == 5
    assert inst.overridden() == True
    assert m.Abstract.static_method() == m.DerivedFromAbstract.static_method()
    assert m.Abstract.static_method() == True
