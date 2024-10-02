# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import readme as m


def test_readme_example(capsys):
    obj = m.Example()
    assert obj.something == 0
    obj.something = 42
    assert obj.something == 42
    assert obj.calculate() == -42  # default argument
    assert obj.calculate(m.Flavor.bland) == 42

    assert m.Example.__doc__ == "A contrived example."

    print(m.Flavor.__doc__)
    output = capsys.readouterr()
    expected = """\
Describes how the output will taste.

Members:

  bland : Like you would expect.

  fruity : It tastes different.
"""
    assert output.out == expected

    help(obj.calculate)
    output = capsys.readouterr()
    expected = """\
Help on method calculate in module readme:

calculate(...) method of readme.Example instance
    calculate(self: readme.Example, flavor: readme.Flavor = <Flavor.fruity: 1>) -> int

    Do a complicated calculation.

"""
    assert output.out == expected
