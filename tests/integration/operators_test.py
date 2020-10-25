import operators as m

import inspect


assert (not NotImplemented) is False


def get_proper_members(cls, predicate=None):
    mro = cls.mro()
    assert mro[-2].__class__.__name__ == "pybind11_type"
    return {
        attr.name: attr.object
        for attr in inspect.classify_class_attrs(cls)
        if attr.defining_class in mro[:-2]
        and (predicate is None or predicate(attr.object))
    }


def test_only_expected_operators_are_defined():
    assert sorted(get_proper_members(m.Other, callable).keys()) == [
        "__init__",
    ]

    assert sorted(get_proper_members(m.Number, callable).keys()) == [
        "__eq__",
        "__gt__",
        "__init__",
        "__lt__",
    ]

    assert sorted(get_proper_members(m.Integer, callable).keys()) == [
        "__eq__",
        "__gt__",
        "__init__",
        "__lt__",
    ]

    assert sorted(get_proper_members(m.Templated, callable).keys()) == [
        "__eq__",
        "__init__",
    ]


def test_member_function_with_same_type_rhs_by_value():
    inst = m.Number(5)
    assert inst == m.Number(5)
    assert not inst == m.Number(2)


def test_member_function_with_builtin_type_rhs_by_value():
    inst = m.Number(5)
    assert inst > 2
    assert not inst > 123


def test_inline_friend_with_same_type_by_ref():
    inst = m.Number(5)
    assert inst < m.Number(7)
    assert not inst < m.Number(3)


def test_inline_friend_with_builtin_type_by_value():
    inst = m.Number(5)
    assert inst == 5
    assert not inst == 1


def test_reverse_inline_friend_with_custom_type_by_value():
    inst = m.Number(5)
    other = m.Other()
    assert not other == inst
    assert not inst.__eq__(other)
    assert not inst == other
    assert other.__eq__(inst) is NotImplemented


def test_reverse_inline_friend_with_builtin_type_by_value():
    inst = m.Number(5)
    assert 7 > inst
    assert inst.__lt__(7)
    assert not 3 > inst
    assert not inst.__lt__(3)


def test_inline_friend_with_builtin_type():
    inst = m.Number(5)
    assert not inst == True
    assert not inst.__eq__(True)


def test_private_operator_is_not_exposed():
    assert m.Integer(5).__eq__(m.Number(5)) is NotImplemented


def test_in_associated_namespace():
    inst = m.Integer(123)
    assert inst == 123
    assert not inst == 5
    assert inst.__eq__(123)
    assert not inst.__eq__(5)


def test_reverse_in_associated_namespace():
    inst = m.Integer(123)
    assert 5 < inst
    assert inst.__gt__(5)
    assert not 321 < inst
    assert not inst.__gt__(321)


def test_instantiated_template_operators():
    inst = m.Templated(321)
    assert inst == 321
    assert not inst == 123
    assert inst == m.Templated(321)
    assert not inst == m.Templated(123)
