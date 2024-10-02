# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import parameters as m
import pytest


def test_parameter_names_are_part_of_signature():
    assert (
        "are: bool, part: int, of: int, the_signature: float"
        in m.parameter_names.__doc__
    )
    assert ": bool, not_a: bool, : bool, problem: bool" in m.missing_names.__doc__
    assert "also: bool, here: bool" in m.Example.__init__.__doc__
    assert "it: bool, works: bool" in m.Example.method.__doc__


def test_keyword_arguments_can_be_used_in_call():
    assert m.return_second(2, second=5) == 5
    assert m.return_second(second=4, first=1) == 4


def test_cannot_pass_none_to_required_arguments():
    valid = m.Example()
    inst = m.TakesPointers()

    for inst in [m, m.TakesPointers()]:
        inst.accepts_none(None)
        inst.accepts_none(valid)

        inst.required(valid)
        with pytest.raises(TypeError, match="incompatible function arguments"):
            inst.required(None)

        inst.required_second(valid, valid)
        inst.required_second(None, valid)
        with pytest.raises(TypeError, match="incompatible function arguments"):
            inst.required_second(None, None)

        inst.required_both(valid, valid)
        with pytest.raises(TypeError, match="incompatible function arguments"):
            inst.required_both(None, None)

        with pytest.raises(TypeError, match="incompatible function arguments"):
            inst.required_both(valid, None)

        with pytest.raises(TypeError, match="incompatible function arguments"):
            inst.required_both(None, valid)


def test_overloading_works():
    for inst in [m, m.TakesDouble()]:
        assert inst.overload_is_double(123) is False
        assert inst.overload_is_double(1234.5) is True


def test_conversion_is_enabled_by_default():
    for inst in [m, m.TakesDouble()]:
        assert inst.normal(123) == 123.0
        assert inst.normal(1234.5) == 1234.5


def test_noconvert_prevents_implicit_conversion():
    for inst in [m, m.TakesDouble()]:
        assert inst.noconvert(5.0) == 5.0
        with pytest.raises(TypeError, match="incompatible function arguments"):
            inst.noconvert(5)

        assert inst.noconvert_second(5.0, 2.5) == 7.5
        assert inst.noconvert_second(5, 2.5) == 7.5
        with pytest.raises(TypeError, match="incompatible function arguments"):
            inst.noconvert_second(5.5, 2)

        assert inst.noconvert_both(5.0, 2.5) == 7.5
        with pytest.raises(TypeError, match="incompatible function arguments"):
            inst.noconvert_both(5, 2)
        with pytest.raises(TypeError, match="incompatible function arguments"):
            inst.noconvert_both(5, 2.5)
        with pytest.raises(TypeError, match="incompatible function arguments"):
            inst.noconvert_both(5.5, 2)
