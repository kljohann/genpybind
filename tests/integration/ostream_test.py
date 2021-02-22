import ostream as m

import re


def test_example_has_repr_and_str():
    inst = m.Example(123)
    assert repr(inst) == "Example(123)"
    assert str(inst) == repr(inst)
    assert not hasattr(inst, "__lshift__")


def test_member_functions_are_ignored():
    inst = m.RedHerring()
    assert re.match("<ostream.RedHerring object at .+?>", repr(inst))
    assert str(inst) == repr(inst)
    assert "std::ostream" in inst.__lshift__.__doc__


def test_can_be_exposed_as_str():
    inst = m.ExposedAsStr()
    assert str(inst) == "human readable"
    assert re.match("<ostream.ExposedAsStr object at .+?>", repr(inst))
    assert not hasattr(inst, "__lshift__")


def test_can_be_exposed_as_repr():
    inst = m.ExposedAsRepr()
    assert repr(inst) == "ExposedAsRepr()"
    assert str(inst) == repr(inst)
    assert not hasattr(inst, "__lshift__")
