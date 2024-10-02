# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

import enums_export_values_when_scoped as m


def test_values_are_exported():
    for name in ["ScopedExport", "ScopedExportTrue"]:
        enum = getattr(m, name)
        for suffix in ["A", "B", "C"]:
            assert getattr(m, f"{name}{suffix}") == getattr(enum, f"{name}{suffix}")


def test_values_are_not_exported():
    for name in ["Scoped", "ScopedExportFalse"]:
        enum = getattr(m, name)
        for suffix in ["A", "B", "C"]:
            assert not hasattr(m, f"{name}{suffix}")
            assert hasattr(enum, f"{name}{suffix}")
