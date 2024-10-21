// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

#define GENPYBIND_VISIBLE GENPYBIND(visible)
#define GENPYBIND_MODULE GENPYBIND(module)

namespace example GENPYBIND_VISIBLE {
bool visible_in_first();
bool hidden_in_first() GENPYBIND(hidden);
} // namespace example

namespace example GENPYBIND_VISIBLE {
bool visible_in_second();
bool hidden_in_second() GENPYBIND(hidden);
} // namespace example

namespace submodule GENPYBIND_MODULE {
bool visible_in_first() GENPYBIND(visible);
} // namespace submodule

namespace submodule GENPYBIND_MODULE {
bool visible_in_second() GENPYBIND(visible);
} // namespace submodule

namespace nothing_exposed_in_original_ns {}

namespace nothing_exposed_in_original_ns {
struct GENPYBIND(visible) ButOnlyLater {};
} // namespace nothing_exposed_in_original_ns

namespace nothing_exposed_in_original_ns {
GENPYBIND(visible)
bool and_later();
} // namespace nothing_exposed_in_original_ns
