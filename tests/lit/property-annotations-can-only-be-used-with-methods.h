// RUN: genpybind-tool --xfail %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

// CHECK: methods.h:[[# @LINE + 4]]:5: error: Invalid annotation for free function: getter_for("something")
// CHECK-NEXT: int free_function_cannot_be_getter();
// CHECK-NEXT: ~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
GENPYBIND(getter_for("something"))
int free_function_cannot_be_getter();

// CHECK: methods.h:[[# @LINE + 4]]:6: error: Invalid annotation for free function: setter_for("something")
// CHECK-NEXT: void free_function_cannot_be_setter(int value);
// CHECK-NEXT: ~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
GENPYBIND(setter_for("something"))
void free_function_cannot_be_setter(int value);

struct Something {
  // CHECK: methods.h:[[# @LINE + 2]]:3: error: Invalid annotation for constructor: getter_for("property")
  GENPYBIND(getter_for("property"))
  Something(int);

  // CHECK: methods.h:[[# @LINE + 2]]:3: error: Invalid annotation for constructor: setter_for("property")
  GENPYBIND(setter_for("property"))
  Something(double);

  // CHECK: methods.h:[[# @LINE + 3]]:3: error: Invalid annotation for named declaration: getter_for("property")
  // CHECK: methods.h:[[# @LINE + 2]]:3: error: Invalid annotation for named declaration: setter_for("property")
  GENPYBIND(getter_for("property"), setter_for("property"))
  operator bool() const;

  // CHECK: methods.h:[[# @LINE + 3]]:3: error: Invalid annotation for named declaration: getter_for("property")
  // CHECK: methods.h:[[# @LINE + 2]]:3: error: Invalid annotation for named declaration: setter_for("property")
  GENPYBIND(getter_for("property"), setter_for("property"))
  ~Something();
};

template <typename T>
struct Example {};

// CHECK: methods.h:[[# @LINE + 3]]:1: error: Invalid annotation for named declaration: getter_for("property")
// CHECK: methods.h:[[# @LINE + 2]]:1: error: Invalid annotation for named declaration: setter_for("property")
GENPYBIND(getter_for("property"), setter_for("property"))
Example(float) -> Example<double>;

// CHECK: 10 errors generated.