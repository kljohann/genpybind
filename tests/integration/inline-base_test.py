# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import inline_base as m
from helpers import get_proper_members, get_user_mro


def test_inlining_preserves_decls_but_not_inheritance():
    inst = m.InlineBase()
    assert not isinstance(inst, m.Base)
    assert inst.from_base()


def test_names_in_the_derived_class_win_during_lookup():
    inst = m.InlineBase()
    assert not m.Base().hidden()
    assert inst.hidden()


def test_it_is_possible_to_derive_from_an_inlined_class():
    assert issubclass(m.DerivedFromInlineBase, m.InlineBase)
    assert not issubclass(m.DerivedFromInlineBase, m.Base)
    inst = m.DerivedFromInlineBase()
    assert isinstance(inst, m.InlineBase)
    assert not isinstance(inst, m.Base)
    assert inst.from_base()
    assert inst.hidden()


def test_inlining_a_class_that_uses_inlining():
    assert not issubclass(m.InlineInlineBase, m.InlineBase)
    assert not issubclass(m.InlineInlineBase, m.Base)
    assert get_user_mro(m.InlineInlineBase) == [m.InlineInlineBase]
    inst = m.InlineInlineBase()
    assert not isinstance(inst, m.InlineBase)
    assert not isinstance(inst, m.Base)
    assert inst.from_base()
    assert inst.hidden()


def test_inlining_a_class_that_hides_its_base():
    assert not issubclass(m.InlineHideBase, m.HideBase)
    assert not issubclass(m.InlineHideBase, m.Base)
    assert get_user_mro(m.InlineHideBase) == [m.InlineHideBase]
    inst = m.InlineHideBase()
    assert not isinstance(inst, m.HideBase)
    assert not isinstance(inst, m.Base)
    assert not hasattr(inst, "from_base")
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


def test_all_bases_are_inlined_when_used_without_arguments():
    assert not issubclass(m.InlineAll, m.Base)
    inst = m.InlineAll()
    assert not isinstance(inst, m.Base)
    assert get_user_mro(m.InlineAll) == [m.InlineAll]
    assert inst.from_base()
    assert inst.from_other()


def test_inlining_an_intermediate_base_preserves_inheritance_for_further_bases():
    inst = m.InlineDerived()
    assert issubclass(m.Derived, m.Base)
    assert issubclass(m.InlineDerived, m.Base)
    assert not issubclass(m.InlineDerived, m.Derived)
    assert isinstance(inst, m.Base)
    assert not isinstance(inst, m.Derived)
    assert inst.from_base()
    assert inst.from_derived()


def test_can_inline_bases_of_an_intermediate_base_without_inlining_the_intermediate_base():
    assert issubclass(m.InlineBaseKeepDerived, m.Derived)
    # NOTE: Although the methods of Base have been inlined into
    # `InlineBaseKeepDerived`, it is still considered a subclass of `Base`,
    # since it is registered as a subclass of `Derived`, which itself is
    # a subclass of `Base`.
    assert issubclass(m.Derived, m.Base)
    assert issubclass(m.InlineBaseKeepDerived, m.Base)

    assert sorted(
        get_proper_members(m.InlineBaseKeepDerived, callable, depth=1).keys()
    ) == [
        "__init__",
        "from_base",
        "hidden",
    ]

    inst = m.InlineBaseKeepDerived()
    assert isinstance(inst, m.Base)
    assert isinstance(inst, m.Derived)
    assert inst.from_base()
    assert inst.from_derived()


def test_bases_of_an_intermediate_base_are_also_inlined_if_specified():
    assert sorted(
        get_proper_members(m.InlineBaseAndDerived, callable, depth=1).keys()
    ) == [
        "__init__",
        "from_base",
        "from_derived",
        "hidden",
    ]

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


class TestCRTP:
    def test_helpers_are_hidden(self):
        """CRTP helpers are “hidden”, but base inheritance is preserved."""
        for klass in [
            m.TwoLevelCRTPInlineSecond,
            m.TwoLevelCRTPInlineBoth,
        ]:
            assert get_user_mro(klass) == [klass, m.Base]
        # But without inlining, the class is not considered as inherited from
        # `Base`, since the intermediate levels in the hierarchy are hidden
        # and only immediate base classes are registered with pybind11.
        for klass in [
            m.TwoLevelCRTPNoInline,
            m.TwoLevelCRTPInlineFirst,
        ]:
            assert get_user_mro(klass) == [klass]

    def test_without_annotation_on_most_derived_class_nothing_is_inlined(self):
        inst = m.TwoLevelCRTPNoInline()
        assert not hasattr(inst, "from_crtp")
        assert not hasattr(inst, "from_second_level_crtp")

    def test_can_explicitly_inline_distant_ancestor(self):
        inst = m.TwoLevelCRTPInlineFirst()
        assert hasattr(inst, "from_crtp")
        assert not hasattr(inst, "from_second_level_crtp")

    def test_annotation_on_inlined_base_is_taken_into_account(self):
        inst = m.TwoLevelCRTPInlineSecond()
        assert hasattr(inst, "from_crtp")
        assert hasattr(inst, "from_second_level_crtp")
        # The same behavior as if all `inline_base` annotations were moved to
        # the most derived class:
        inst = m.TwoLevelCRTPInlineBoth()
        assert hasattr(inst, "from_crtp")
        assert hasattr(inst, "from_second_level_crtp")

    def test_inlining_works_for_friended_operators(self):
        assert sorted(
            get_proper_members(m.WithOperatorMixins, callable, depth=1).keys()
        ) == [
            "__eq__",
            "__init__",
            "__invert__",
            "__ne__",
            "__str__",
        ]
        inst = m.WithOperatorMixins(123)
        other = m.WithOperatorMixins(321)
        assert str(inst) == "The value is 123"
        assert inst != other
        assert not inst == other
        assert (~inst).value == -5
