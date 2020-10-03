#pragma once

#include "genpybind.h"

#define GENPYBIND_VISIBLE GENPYBIND(visible)
#define GENPYBIND_MODULE GENPYBIND(module)

namespace example GENPYBIND_VISIBLE {
bool visible_in_first();
bool hidden_in_first() GENPYBIND(hidden);
} // namespace example

namespace example {
bool visible_in_second() GENPYBIND(visible);
bool hidden_in_second();
} // namespace example

namespace submodule GENPYBIND_MODULE {
bool visible_in_first() GENPYBIND(visible);
} // namespace submodule

namespace submodule GENPYBIND_MODULE {
bool visible_in_second() GENPYBIND(visible);
} // namespace submodule
