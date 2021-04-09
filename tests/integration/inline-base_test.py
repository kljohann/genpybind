import inline_base as m
from helpers import get_user_mro


def test_inlining_preserves_decls_but_not_inheritance():
    inst = m.InlineBase()
    assert not isinstance(inst, m.Base)
    assert inst.from_base()


def test_names_in_the_derived_class_win_during_lookup():
    inst = m.InlineBase()
    assert not m.Base().hidden()
    assert inst.hidden()


def test_it_is_possible_to_derive_from_an_inlined_class():
    inst = m.DerivedFromInlineBase()
    assert isinstance(inst, m.InlineBase)
    assert not isinstance(inst, m.Base)
    assert inst.from_base()
    assert inst.hidden()


def test_not_all_bases_need_to_be_inlined():
    assert not hasattr(m, "Other")
    assert get_user_mro(m.InlineOther) == [m.InlineOther, m.Base]
    inst = m.InlineOther()
    assert isinstance(inst, m.Base)
    assert inst.from_other()


def test_several_bases_can_be_inlined():
    inst = m.InlineBoth()
    assert not isinstance(inst, m.Base)
    assert get_user_mro(m.InlineBoth) == [m.InlineBoth]
    assert inst.from_base()
    assert inst.from_other()


def test_inlining_an_intermediate_base_ignores_further_bases():
    inst = m.InlineDerived()
    assert not isinstance(inst, m.Base)
    assert not isinstance(inst, m.Derived)
    assert not hasattr(inst, "from_base")
    assert inst.from_derived()


def test_bases_of_an_intermediate_base_are_also_inlined_if_specified():
    inst = m.InlineBaseAndDerived()
    assert not isinstance(inst, m.Base)
    assert not isinstance(inst, m.Derived)
    assert inst.from_base()
    assert inst.from_derived()


def test_if_an_intermediate_base_is_hidden_its_bases_are_not_inlined():
    inst = m.InlineBaseHideDerived()
    assert not isinstance(inst, m.Base)
    assert not isinstance(inst, m.Derived)
    assert not hasattr(inst, "from_base")
    assert not hasattr(inst, "from_derived")


def test_if_decls_from_several_base_classes_conflict_all_are_exposed_and_pybind11_picks_one():
    inst = m.InlineConflict()
    assert inst.number() in [1, 2]


def test_base_classes_from_nested_namespace_can_be_inlined():
    for klass in [m.InlineNestedBase, m.InlineNestedBaseShort]:
        assert get_user_mro(klass) == [klass]
        inst = klass()
        assert inst.from_nested_base()
