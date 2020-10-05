import submodules as m


def test_last_name_wins():
    assert m.one.X is m.X
    assert m.deux.X is m.X
    assert not hasattr(m, "two")
    assert m.drei.X is m.X
    assert not hasattr(m, "three")
    assert m.quatre.X is m.X
    assert not hasattr(m, "four") and not hasattr(m, "vier")
    assert m.cinq.X is m.X
    assert not hasattr(m, "five") and not hasattr(m, "fuenf")
    assert m.seis.X is m.X
    assert not hasattr(m, "six") and not hasattr(m, "sechs")
