import typedefs_across_modules_definition as m

import inspect


def test_only_expected_decls_are_available():
    assert sorted([k for k, _ in inspect.getmembers(m, inspect.isclass)]) == [
        "Alias",
        "Definition",
        "WorkingEncouragedFromOtherModule",
    ]
    assert m.Alias is m.Definition
