#pragma once

#include "genpybind.h"

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

private:
  void private_method();
};

struct GENPYBIND(visible) ExoticMemberFunctions {
  typedef int FunctionType();

  FunctionType function;

  typedef int OtherFunctionType() const;
  OtherFunctionType other_function;
};
