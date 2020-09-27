#pragma once

#include "genpybind.h"

#define GENPYBIND_VISIBLE GENPYBIND(visible)

/// Inverts the specified `value`.
bool invert(bool value) GENPYBIND(visible);

int add(int lhs, int rhs) GENPYBIND(visible);
double add(double lhs, double rhs) GENPYBIND(visible);

int old_name() GENPYBIND(expose_as("new_name"));

namespace example GENPYBIND_VISIBLE {
bool visible();
bool hidden() GENPYBIND(hidden);
} // namespace example

bool not_exposed();
