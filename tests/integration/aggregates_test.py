# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import aggregates as m
import pytest


def test_aggregates_are_default_constructible():
    agg = m.Aggregate()
    assert (agg.a, agg.b, agg.c) == (0, 0, 0)

    agg = m.WithDefaultInitializers()
    assert agg.lucky_number == 5
    assert agg.it_just_works


def test_aggregates_support_keyword_arguments():
    agg = m.Aggregate(a=1, b=2, c=3)
    assert (agg.a, agg.b, agg.c) == (1, 2, 3)

    agg = m.WithDefaultInitializers(lucky_number=42, it_just_works=False)
    assert agg.lucky_number == 42
    assert not agg.it_just_works


def test_aggregates_enforce_keyword_arguments_for_fields():
    with pytest.raises(TypeError, match="incompatible constructor arguments"):
        _ = m.Aggregate(4, 5, 6)

    # TODO: Maybe make an exception if there is only a single field?
    with pytest.raises(TypeError, match="incompatible constructor arguments"):
        _ = m.CamelCase(4)


def test_omitted_fields_are_implicitly_initialized():
    agg = m.Aggregate(a=1)
    assert (agg.a, agg.b, agg.c) == (1, 0, 0)
    agg = m.Aggregate(a=1, c=1)
    assert (agg.a, agg.b, agg.c) == (1, 0, 1)

    obj = m.WithBases(d=5)
    assert (obj.a, obj.b, obj.c, obj.CamelField, obj.d) == (0, 0, 0, 0, 5)


def test_base_classes_can_be_passed_in_as_ctor_args():
    agg = m.Aggregate(a=1, b=2, c=3)
    camel = m.CamelCase(CamelField=4)

    obj = m.WithBases(agg, camel, d=5)
    assert (obj.a, obj.b, obj.c, obj.CamelField, obj.d) == (1, 2, 3, 4, 5)

    # Parameters for bases are position-only:
    with pytest.raises(TypeError, match="incompatible constructor arguments"):
        _ = m.WithBases(Aggregate=agg, CamelCase=camel, d=5)


def test_inlined_and_hidden_bases_are_not_supported_yet():
    assert "3." in m.WithBases.__init__.__doc__
    # Only two overloads: Default constructor and copy constructor.
    assert "3." not in m.WithInlinedBase.__init__.__doc__
    assert "3." not in m.WithHiddenBase.__init__.__doc__
