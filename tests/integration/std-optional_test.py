# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import std_optional as m


def test_parameters_can_be_optional():
    assert "(value: Optional[int] = None)" in m.example.__doc__
    m.example(123)
    m.example()
    m.example(None)


def test_return_values_can_be_optional():
    assert " -> Optional[int]" in m.example.__doc__
    assert m.example(123) == 123
    assert m.example() is None
    assert m.example(None) is None
