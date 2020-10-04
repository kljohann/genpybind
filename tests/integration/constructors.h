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

struct GENPYBIND(visible) Implicit {
  Implicit(int value) GENPYBIND(implicit_conversion) : value_(value) {}
  Implicit(Example example) GENPYBIND(implicit_conversion)
      : value_(example.value()) {}

  int value() const { return value_; }
  int value_;
};

GENPYBIND(visible)
int accepts_implicit(Implicit value);

GENPYBIND(noconvert(value))
int noconvert_implicit(Implicit value);

struct GENPYBIND(visible) AcceptsImplicit {
  AcceptsImplicit(Implicit value) : value_(value.value()) {}

  int value() const { return value_; }
  int value_;
};

struct GENPYBIND(visible) NoconvertImplicit {
  GENPYBIND(noconvert(value))
  NoconvertImplicit(Implicit value) : value_(value.value()) {}

  int value() const { return value_; }
  int value_;
};

struct GENPYBIND(visible) ImplicitRef {
  ImplicitRef(const Example &example) GENPYBIND(implicit_conversion)
      : value_(example.value()) {}

  int value() const { return value_; }
  int value_;
};

GENPYBIND(visible)
int accepts_implicit_ref(ImplicitRef value);

struct GENPYBIND(visible) ImplicitPtr {
  ImplicitPtr(const Example *example) GENPYBIND(implicit_conversion)
      : value_(example == nullptr ? -1 : example->value()) {}

  int value() const { return value_; }
  int value_;
};

GENPYBIND(visible)
int accepts_implicit_ptr(ImplicitPtr value);
