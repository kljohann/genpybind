import methods as m


def test_can_have_docstring():
    assert "A brief docstring." in m.Example.__doc__
    assert "Another brief docstring." in m.Example.public_method.__doc__


def test_public_methods_are_exposed():
    inst = m.Example()
    assert inst.public_method() == 5
    assert inst.public_const_method() == 123


def test_methods_can_be_renamed():
    inst = m.Example()
    assert not hasattr(inst, "old_name")
    assert inst.new_name() == 42


def test_methods_can_be_overloaded():
    inst = m.Example()
    assert inst.overloaded(5) == 5
    assert isinstance(inst.overloaded(5), int)
    assert inst.overloaded(2.5) == 2.5


def test_methods_can_be_static():
    assert isinstance(m.Example.static_method(), m.Example)


def test_hidden_methods_are_absent():
    inst = m.Example()
    assert not hasattr(inst, "hidden_method")


def test_private_methods_are_absent():
    inst = m.Example()
    assert not hasattr(inst, "private_method")


def test_deleted_methods_are_not_exposed():
    inst = m.Example()
    assert not hasattr(m, "deleted_method")


def test_using_typedefs_for_member_functions_works():
    assert m.ExoticMemberFunctions().function() == 42
    assert m.ExoticMemberFunctions().other_function() == 123
