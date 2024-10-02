# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import pytest


@pytest.fixture(scope="session")
def module_with_warnings():
    with pytest.warns(Warning) as warnings_recorder:
        import typedefs_and_aliases as m
    yield m, warnings_recorder
    assert len(warnings_recorder) == 0, "not all warnings acknowledged via .pop()"


def test_should_warn_about_unexposed_targets(module_with_warnings):
    m, warnings_recorder = module_with_warnings

    # Warning message is currently emitted once for every use.
    for _ in range(2):
        warning = warnings_recorder.pop()
        assert "Reference to unknown type 'UnexposedTarget'" in str(warning.message)

    assert not hasattr(m, "UnexposedTarget")
    assert m.using_unexposed is None
    assert m.typedef_unexposed is None


def test_only_explicitly_visible_aliases_are_exposed(module_with_warnings):
    m, _ = module_with_warnings
    for parent in [m, m.VisibleParent]:
        for prefix in ["using", "typedef"]:
            for suffix in ["unannotated", "visible", "defaulted", "hidden"]:
                name = f"{prefix}_{suffix}"
                if suffix == "visible":
                    assert hasattr(parent, name)
                    assert getattr(parent, name) == m.Target
                else:
                    assert not hasattr(parent, name)


def test_forward_declared_target(module_with_warnings):
    m, _ = module_with_warnings
    assert m.using_forward_declared is m.ForwardDeclaredTarget
    assert m.typedef_forward_declared is m.ForwardDeclaredTarget
