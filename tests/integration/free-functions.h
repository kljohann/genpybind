// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

#define GENPYBIND_VISIBLE GENPYBIND(visible)

/// Inverts the specified `value`.
bool invert(bool value) GENPYBIND(visible);

int add(int lhs, int rhs) GENPYBIND(visible);
double add(double lhs, double rhs) GENPYBIND(visible);

int old_name() GENPYBIND(expose_as("new_name"));

namespace example GENPYBIND_VISIBLE {
bool visible();
bool hidden() GENPYBIND(hidden);
bool deleted() = delete;
} // namespace example

bool not_exposed();

int missing_param_names(int, int) GENPYBIND(visible);
