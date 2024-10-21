// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

enum GENPYBIND(visible) Unscoped { UnscopedA, UnscopedB, UnscopedC };

enum GENPYBIND(export_values) UnscopedExport {
  UnscopedExportA,
  UnscopedExportB,
  UnscopedExportC,
};

enum GENPYBIND(export_values(true)) UnscopedExportTrue {
  UnscopedExportTrueA,
  UnscopedExportTrueB,
  UnscopedExportTrueC,
};

enum GENPYBIND(export_values(false)) UnscopedExportFalse {
  UnscopedExportFalseA,
  UnscopedExportFalseB,
  UnscopedExportFalseC,
};
