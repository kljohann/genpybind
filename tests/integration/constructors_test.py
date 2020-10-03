import constructors as m

import pytest


def test_can_have_docstring():
    assert "A brief docstring." in m.Example.__init__.__doc__
    assert (
        "Another brief docstring. A default flag value is used."
        in m.Example.__init__.__doc__
    )


def test_can_be_overloaded():
    inst = m.Example(12)
    assert inst.value() == 12
    assert inst.flag() == False

    inst = m.Example(123, True)
    assert inst.value() == 123
    assert inst.flag() == True


def test_parameter_names_are_part_of_signature():
    assert ", value: int)" in m.Example.__init__.__doc__
    assert ", value: int, flag: bool)" in m.Example.__init__.__doc__


def test_accept_keyword_arguments():
    inst = m.Example(flag=True, value=42)
    assert inst.flag() == True
    assert inst.value() == 42

    inst = m.Example(value=42)
    assert inst.flag() == False
    assert inst.value() == 42


def test_required_arguments_are_supported():
    valid = m.Example(5)
    m.AcceptsNone(valid)
    m.AcceptsNone(None)
    m.RejectsNone(valid)
    with pytest.raises(TypeError, match="incompatible constructor arguments"):
        m.RejectsNone(None)
