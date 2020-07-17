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
    names = set(name for name in dir(m) if not name.startswith("__"))
    assert names == expected
