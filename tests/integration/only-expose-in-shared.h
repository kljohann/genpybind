// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

#define GENPYBIND_ONLY_IN_A GENPYBIND(only_expose_in(only_expose_in_a))
#define GENPYBIND_ONLY_IN_B GENPYBIND(only_expose_in(only_expose_in_b))
#define GENPYBIND_ONLY_IN_BOTH                                                 \
  GENPYBIND(only_expose_in(only_expose_in_a, only_expose_in_b))

struct GENPYBIND(visible) ExposedEverywhere {};

namespace only_in_a GENPYBIND_ONLY_IN_A {
struct GENPYBIND(visible) ExposedInA {};
} // namespace GENPYBIND_ONLY_IN_A

namespace only_in_b GENPYBIND_ONLY_IN_B {
namespace further_nesting_does_not_matter {
struct GENPYBIND(visible) ExposedInB {};
} // namespace further_nesting_does_not_matter
} // namespace GENPYBIND_ONLY_IN_B

namespace only_in_both GENPYBIND_ONLY_IN_BOTH {
struct GENPYBIND(visible) ExposedInBoth {};
} // namespace GENPYBIND_ONLY_IN_BOTH
