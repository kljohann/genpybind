# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import macros as m


def test_expose_as_accepts_expanded_macro():
    m.OneExposed()
    m.TwoExposed()
