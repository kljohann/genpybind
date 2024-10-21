// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

// NOTE: pybind11 only exposes a limited set of operations for scoped enums (see
// https://github.com/pybind/pybind11/issues/2221), which is why this example
// uses unscoped enumerations.

enum GENPYBIND(arithmetic) Access {
  Read = 4,
  Write = 2,
  Execute = 1,
};

enum GENPYBIND(arithmetic(false)) ExplicitFalse {
  One = 1,
  Two = 2,
  Three = 3,
};

/// Docstrings are also supported.
enum GENPYBIND(arithmetic(true)) ExplicitTrue {
  Four = 4,
  Five = 5,
  Six = 6,
};

enum GENPYBIND(visible) Default {
  Seven = 7,
  Eight = 8,
  Nine = 9,
};
