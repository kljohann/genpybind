# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import expose_here_can_move_enums as m


def test_existence():
    names = {name for name in dir(m) if not name.startswith("__")}
    assert names == {"Outer"}
    names = {name for name in dir(m.Outer) if not name.startswith("__")}
    assert names == {"Scoped", "Unscoped", "Y"}
    assert hasattr(m.Outer.Scoped, "X")
