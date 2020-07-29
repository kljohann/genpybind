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
