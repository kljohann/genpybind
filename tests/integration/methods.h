#pragma once

#include "genpybind.h"

struct GENPYBIND(visible) Example {
  int public_method();
  int public_const_method() const;

  int old_name() const GENPYBIND(expose_as(new_name));

  int overloaded(int value) const;
  double overloaded(double value) const;

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
