#pragma once

#include "genpybind.h"

enum class GENPYBIND(visible) Example {
  Hidden GENPYBIND(hidden),
  Original GENPYBIND(expose_as(Renamed)),
  NotAnnotated,
};
