#pragma once

#include "genpybind.h"

enum class GENPYBIND(visible) Scoped { ScopedA, ScopedB, ScopedC };

enum class GENPYBIND(export_values) ScopedExport {
  ScopedExportA,
  ScopedExportB,
  ScopedExportC,
};

enum class GENPYBIND(export_values(default)) ScopedExportDefault {
  ScopedExportDefaultA,
  ScopedExportDefaultB,
  ScopedExportDefaultC,
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
