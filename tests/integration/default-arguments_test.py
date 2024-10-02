# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import default_arguments as m
import pytest

EXPECTED = {
    1: 123,
    2: None,
    3: 3,
    4: 4,
    5: 1234,
    6: 1234,
    7: 456,
    8: 456,
    9: 456,
    10: 456,
    11: 0,
    12: 0,
    13: 5,
    14: 5,
    15: 1234,
    16: 6,
    17: 123,
    18: 123,
    19: 123,
    20: 123,
    21: 123,
    22: 123,
    23: 123,
    24: 123,
    25: 123,
}


@pytest.mark.parametrize("index", EXPECTED.keys())
def test_function_can_be_called_without_arguments(index):
    if index == max(EXPECTED.keys()):
        assert not hasattr(m, f"function_{index + 1:02}")
    if index == 15:
        # TODO: Investigate pointer default arguments
        # (`munmap_chunk(): invalid pointer`)
        pytest.xfail("pointer default argument is broken")
    if index == 24:
        # TODO: Investigate reference template parameter
        # (gets expanded to `Reference<&nested::global_a>`)
        pytest.xfail("reference template parameter is broken")
    name = f"function_{index:02}"
    function = getattr(m, name)
    assert function() == EXPECTED[index]


def test_template_functions_can_be_called_without_arguments():
    for index in range(1, 6):
        name = f"template_function_{index:02}"
        function = getattr(m, name)
        assert function() == 123
    assert not hasattr(m, f"template_function_{index + 1:02}")


def test_member_function_of_template_can_be_called_without_arguments():
    assert m.Template_Example_().member_function() == 123


def test_not_all_parameters_need_to_have_default_values():
    pass
