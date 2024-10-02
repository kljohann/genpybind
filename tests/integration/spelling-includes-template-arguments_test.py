# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import spelling_includes_template_arguments as m


def test_expected_instantiations_are_present():
    expected = {
        "Encouraged_123_",
        "Encouraged_42_",
        "Encouraged_with_types_arguments_X_",
        "Encouraged_with_types_arguments_Y_",
        "WithInteger_123_",
        "WithInteger_42_",
        "WithType_with_types_arguments_X_",
        "WithType_with_types_arguments_Y_",
    }
    names = {name for name in dir(m) if name[0].isupper() and not name.startswith("__")}
    assert names == expected


def test_aliases_are_present():
    assert m.WithInteger_123_.Alias == m.Encouraged_123_
    assert m.WithInteger_42_.Alias == m.Encouraged_42_
    assert (
        m.WithType_with_types_arguments_X_.Alias == m.Encouraged_with_types_arguments_X_
    )
    assert (
        m.WithType_with_types_arguments_Y_.Alias == m.Encouraged_with_types_arguments_Y_
    )
