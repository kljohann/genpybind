# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import os

import manual_order_of_execution  # noqa: F401


def test_order_of_execution_matches_context_graph_hierarchy():
    order_of_execution = os.environ.get("order_of_execution", "").split(",")
    # Declaration contexts are sorted in order of their dependencies.
    # Within a context, declarations are sorted by their order in the TU.
    assert order_of_execution == [
        # `TranslationUnitDecl` is set up first.
        "",
        ":: [early]",
        ":: [before postamble]",
        ":: [after postamble]",
        # All manual bindings from `reopened_namespace`, whose original
        # namespace declaration comes before all other declarations in the file.
        "::reopened_namespace [first]",
        "::reopened_namespace [second]",
        "::reopened_namespace [third]",
        # All manual bindings in `nested` are emitted together and thus come
        # before those from nested declaration contexts.
        "::nested [before Example]",
        "::nested [after Example]",
        # `::nested::Example` comes before `::nested::nested`, because they have
        # no interdependence and `Example` occurs earlier in the TU.
        "::nested::Example [first]",
        "::nested::Example [second]",
        "::nested::nested",
        # The same is true for `::Toplevel` in relation to `::nested`: Both only
        # depend on the exposed TU context, but `nested` occurs earlier.
        "::Toplevel",
        # `postamble` manual bindings come last.
        ":: [first postamble]",
        ":: [second postamble]",
    ]
