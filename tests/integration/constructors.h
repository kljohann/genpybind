// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

struct GENPYBIND(visible) Example {
  /// A brief docstring.
  Example(int value, bool flag);
  /// Another brief docstring.  A default flag value is used.
  Example(int value);

  int value;
  bool flag;
};

struct GENPYBIND(visible) AcceptsNone {
  AcceptsNone(const Example *example);
};

struct GENPYBIND(visible) RejectsNone {
  GENPYBIND(required(example))
  RejectsNone(const Example *example);
};

struct GENPYBIND(visible) Onetwothree {};

struct GENPYBIND(visible) Implicit {
  Implicit(int value) GENPYBIND(implicit_conversion) : value(value) {}
  Implicit(Example example) GENPYBIND(implicit_conversion)
      : value(example.value) {}
  explicit Implicit(Onetwothree) GENPYBIND(implicit_conversion) : value(123) {}

  int value;
};

GENPYBIND(visible)
int accepts_implicit(Implicit value);

GENPYBIND(noconvert(value))
int noconvert_implicit(Implicit value);

struct GENPYBIND(visible) AcceptsImplicit {
  AcceptsImplicit(Implicit value) : value(value.value) {}

  int value;
};

struct GENPYBIND(visible) NoconvertImplicit {
  GENPYBIND(noconvert(value))
  NoconvertImplicit(Implicit value) : value(value.value) {}

  int value;
};

struct GENPYBIND(visible) ImplicitRef {
  ImplicitRef(const Example &example) GENPYBIND(implicit_conversion)
      : value(example.value) {}

  int value;
};

GENPYBIND(visible)
int accepts_implicit_ref(ImplicitRef value);

struct GENPYBIND(visible) ImplicitPtr {
  ImplicitPtr(const Example *example) GENPYBIND(implicit_conversion)
      : value(example == nullptr ? -1 : example->value) {}

  int value;
};

GENPYBIND(visible)
int accepts_implicit_ptr(ImplicitPtr value);
