# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import pytest

with pytest.warns(Warning) as warnings_recorder:
    import typedefs_across_modules as m

# NOTE: Imported _after_ dependant module, so if there were no warnings the
# import statement had the intended effect.
import typedefs_across_modules_definition as m_def


def test_encourage_does_not_work_if_not_part_of_defining_modules_translation_unit():
    warning = warnings_recorder.pop()
    assert (
        len(warnings_recorder) == 0
    ), f"unexpected warnings: {[str(m) for m in warnings_recorder]}"
    assert (
        "Reference to unknown type 'example::nested::EncouragedFromOtherModule'"
        in str(warning.message)
    )
    assert m.AliasToEncouraged is None


def test_aliases_are_available_and_match_definition():
    assert m.AliasOfDefinition is m_def.Definition
    assert m.AliasOfAlias is m_def.Alias
    assert m.AliasOfAlias is m_def.Definition
    assert hasattr(m, "ExposedHere")
    assert not hasattr(m_def, "ExposedInOtherModule")
    assert m.WorkingAliasToEncouraged is m_def.WorkingEncouragedFromOtherModule
