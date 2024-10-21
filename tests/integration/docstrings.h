// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

/// Describes how the output will taste.
enum class GENPYBIND(visible) Flavor {
  /// Like you would expect.
  bland,
  /// It tastes different.
  fruity,
};

/// A contrived example.
///
/// Only the “brief” docstring is used in the Python bindings.
class GENPYBIND(visible) Example {};

/// Also here.
class GENPYBIND(dynamic_attr) Dynamic {};
