// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

#include <ostream>
#include <string>

struct GENPYBIND(visible) Example {
  Example(int value) : value(value) {}
  int value;

  GENPYBIND(expose_as(__str__))
  std::string to_string() const;
};

GENPYBIND(expose_as(__repr__))
std::ostream &operator<<(std::ostream &os, const Example &example);

struct GENPYBIND(visible) ExposedAsStr {
  GENPYBIND(expose_as(__str__))
  friend std::ostream &operator<<(std::ostream &os, const ExposedAsStr &value);
};

struct GENPYBIND(visible) ExposedAsRepr {
  GENPYBIND(expose_as(__repr__))
  friend std::ostream &operator<<(std::ostream &os, const ExposedAsRepr &value);
};

// Ensure that gennpybind is not confused by member functions
// that accept `std::ostream`.

struct GENPYBIND(visible) RedHerring {
  std::ostream &operator<<(std::ostream &os) const;
};

struct GENPYBIND(visible) AnotherRedHerring {
  // NOTE: `expose_as` is currently ignored for non-ostream operators.
  GENPYBIND(expose_as(__str__))
  std::ostream &operator<<(std::ostream &os) const;
};
