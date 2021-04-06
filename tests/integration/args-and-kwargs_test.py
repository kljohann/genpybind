import args_and_kwargs as m


def test_accepts_arbitrary_number_of_arguments():
    assert m.number_of_arguments(1, 2, 3, a=4, b=5, c=6) == 6


def test_mixed_arguments():
    assert m.mixed(10, 1, 2, 3, a=4, b=5, c=6) == 16
    assert m.mixed(a=4, b=5, c=6, offset=10) == 13
