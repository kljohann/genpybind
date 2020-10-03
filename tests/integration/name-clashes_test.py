import name_clashes as m


def test_generated_code_does_not_clash_with_exposed_decls():
    inst = m.derived()
    assert isinstance(inst, m.root)
    m.context()
