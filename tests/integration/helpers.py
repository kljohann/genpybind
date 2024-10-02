# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import inspect


def get_proper_members(cls, predicate=None, *, depth=None):
    """Returns members defined on `cls`, excluding some base classes.

    By default, only members provided by the pybind11 base class and Python are
    excluded (e.g., default operator implementations).  `depth` can be used to
    exclude members starting from that position in the method resolution order
    (e.g., for `depth=1` only members defined on `cls` itself are returned.)

    Parameters:
        predicate: Only return members whose definition fulfills this predicate.
        depth: Only return members this deep into the method resolution order.

    Returns:
        A dictionary mapping the members' names to their definitions.
    """
    mro = cls.mro()
    assert mro[-2].__class__.__name__ == "pybind11_type"
    depth = depth or -2
    return {
        attr.name: attr.object
        for attr in inspect.classify_class_attrs(cls)
        if attr.defining_class in mro[:depth]
        and (predicate is None or predicate(attr.object))
    }


def get_user_mro(cls):
    mro = cls.mro()
    assert mro[-2].__class__.__name__ == "pybind11_type"
    return mro[:-2]
