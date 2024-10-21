// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

enum class GENPYBIND(visible) Scoped { ScopedA, ScopedB, ScopedC };

enum class GENPYBIND(export_values) ScopedExport {
  ScopedExportA,
  ScopedExportB,
  ScopedExportC,
};

enum class GENPYBIND(export_values(true)) ScopedExportTrue {
  ScopedExportTrueA,
  ScopedExportTrueB,
  ScopedExportTrueC,
};

enum class GENPYBIND(export_values(false)) ScopedExportFalse {
  ScopedExportFalseA,
  ScopedExportFalseB,
  ScopedExportFalseC,
};
