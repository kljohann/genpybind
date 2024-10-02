# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import visibility as m


def test_decls_without_annotations_are_not_exposed():
    assert not hasattr(m, "NoAnnotations")
    assert not hasattr(m, "no_annotations")


def test_decls_without_annotation_arguments_are_not_exposed():
    assert not hasattr(m, "NoArguments")
    assert not hasattr(m, "no_arguments")


def test_decls_can_be_visible_explicitly():
    m.ExplicitlyVisible()
    m.explicitly_visible()


def test_decls_can_be_visible_implicitly():
    m.ImplicitlyVisible()
    m.implicitly_visible(123)
