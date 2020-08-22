import expose_here_propagates_visibility_to_nested_contexts as m

import inspect


def test_complete_hierarchy_is_present():
    scope = m
    for level in [
        "ExplicitlyVisible",
        "ImplicitlyVisible",
        "ExposedHere",
        "ShouldAlsoBeVisible",
        None,
    ]:
        classes = {
            k: v
            for k, v in inspect.getmembers(scope, inspect.isclass)
            if not k.startswith("__")
        }
        if level is not None:
            assert set(classes.keys()) == {level}
            scope = classes[level]
        else:
            assert not classes