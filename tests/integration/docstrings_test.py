# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import docstrings as m


def test_enums_support_docstrings():
    expected = """\
Describes how the output will taste.

Members:

  bland : Like you would expect.

  fruity : It tastes different.\
"""
    assert m.Flavor.__doc__ == expected


def test_classes_support_docstrings():
    assert m.Example.__doc__ == "A contrived example."
    # Docstrings also work with extra arguments to `class_`.
    assert m.Dynamic.__doc__ == "Also here."
