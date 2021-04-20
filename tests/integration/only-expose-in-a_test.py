import only_expose_in_a as m

import inspect


def test_only_expected_decls_are_exposed():
    assert sorted([k for k, _ in inspect.getmembers(m, inspect.isclass)]) == [
        "ExposedEverywhere",
        "ExposedInA",
        "ExposedInBoth",
    ]
