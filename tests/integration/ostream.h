#pragma once

#include "genpybind.h"

#include <ostream>

struct GENPYBIND(visible) Example {
  Example(int value) : value(value) {}
  int value;
};

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
