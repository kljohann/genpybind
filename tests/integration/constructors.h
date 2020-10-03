#pragma once

#include "genpybind.h"

struct GENPYBIND(visible) Example {
  /// A brief docstring.
  Example(int value, bool flag);
  /// Another brief docstring.  A default flag value is used.
  Example(int value);

  int value() const;
  bool flag() const;

  int value_;
  bool flag_;
};

struct GENPYBIND(visible) AcceptsNone {
  AcceptsNone(const Example *example);
};

struct GENPYBIND(visible) RejectsNone {
  GENPYBIND(required(example))
  RejectsNone(const Example *example);
};
