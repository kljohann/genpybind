import os

import manual_order_of_execution as m


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
        # TODO: It would be more natural for `nested` to occur before `Toplevel`
        # here, since both only depend on the exposed TU.
        "::Toplevel",
        # All manual bindings in `nested` are emitted together and thus come
        # before those from nested declaration contexts.
        "::nested [before Example]",
        "::nested [after Example]",
        # TODO: `::nested::Example` before `::nested::nested` would also be
        # more natural.
        "::nested::nested",
        "::nested::Example [first]",
        "::nested::Example [second]",
        # `postamble` manual bindings come last.
        ":: [first postamble]",
        ":: [second postamble]",
    ]
