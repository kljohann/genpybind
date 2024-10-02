# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import visibility_propagation_into_nested_namespaces as m


def test_only_visible_classes_are_present():
    expected = {
        "Visible",
        "VisibleInNs",
        "UnannotatedInVisible",
        "VisibleInVisible",
        "DefaultInVisible",
        "WithLinkageInVisible",
        "VisibleInHiddenInVisible",
        "UnannotatedInNsInVisible",
        "VisibleInNsInVisible",
        "DefaultInNsInVisible",
        "WithLinkageInNsInVisible",
        "VisibleInHidden",
    }
    names = {name for name in dir(m) if name[0].isupper() and not name.startswith("__")}
    assert names == expected


def test_only_visible_functions_are_present():
    expected = {
        "visible",
        "visible_in_ns",
        "unannotated_in_visible",
        "visible_in_visible",
        "defaulted_in_visible",
        "with_linkage_in_visible",
        "visible_in_hidden_in_visible",
        "unannotated_in_ns_in_visible",
        "visible_in_ns_in_visible",
        "defaulted_in_ns_in_visible",
        "with_linkage_in_ns_in_visible",
        "visible_in_hidden",
    }
    names = {name for name in dir(m) if name[0].islower() and not name.startswith("__")}
    assert names == expected
