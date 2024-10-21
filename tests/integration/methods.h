// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

struct Example;

struct GENPYBIND(visible) Other {
  GENPYBIND(expose_as(toExample))
  operator Example() const;
};

void consumes_other(Other) GENPYBIND(visible);

/// A brief docstring.
struct GENPYBIND(visible) Example {
  /// Another brief docstring.
  int public_method();
  int public_const_method() const;

  int old_name() const GENPYBIND(expose_as(new_name));

  int overloaded(int value) const;
  double overloaded(double value) const;

  void deleted_method() = delete;
  void hidden_method() GENPYBIND(hidden);

  static Example static_method();

  GENPYBIND(expose_as(__int__))
  operator int() const;

  operator bool() const;

  operator Other() const;

  int operator()() const { return 123; }

  int operator()(int value) const { return value; }

  GENPYBIND(expose_as(call))
  float operator()(float value) const { return value; }

private:
  void private_method();
};

struct GENPYBIND(visible) ExoticMemberFunctions {
  typedef int FunctionType();

  FunctionType function;

  typedef int OtherFunctionType() const;
  OtherFunctionType other_function;
};
