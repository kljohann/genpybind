# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import holder_type as m


def test_can_return_shared_ptr():
    holder = m.Holder()
    assert holder.uses() == 1
    shared = holder.getShared()
    assert holder.uses() == 2
    cloned = shared.clone()
    assert cloned is shared
    assert holder.uses() == 2
    del cloned
    assert holder.uses() == 2
    assert holder.getShared() is shared
    assert holder.uses() == 2
    del shared
    assert holder.uses() == 1
