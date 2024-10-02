# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import explicit_template_instantiations_can_be_exposed as m


def test_classes_are_present():
    for name in [
        "IntSomething",
        "FloatSomething",
        "ExposeSomeInstantiations_bool_",
        "ExposeAll_int_",
        "ExposeAll_float_",
        "BoolSomething",
        "ExposeAll_double_",
    ]:
        assert hasattr(m, name)


def test_names_without_template_arguments_are_not_used():
    assert not hasattr(m, "ExposeSomeInstantiations")
    assert not hasattr(m, "ExposeAll")
