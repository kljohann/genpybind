// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

// CHECK: methods.h:[[# @LINE + 6 ]]:5: error: Invalid annotation for free function: getter_for("something")
// CHECK-NEXT: [[# @LINE + 4 ]] | GENPYBIND(getter_for("something"))
// CHECK-NEXT:                  | ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CHECK-NEXT: [[# @LINE + 3 ]] | int free_function_cannot_be_getter();
// CHECK-NEXT:                  | ~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
GENPYBIND(getter_for("something"))
int free_function_cannot_be_getter();

// CHECK: methods.h:[[# @LINE + 6 ]]:6: error: Invalid annotation for free function: setter_for("something")
// CHECK-NEXT: [[# @LINE + 4 ]] | GENPYBIND(setter_for("something"))
// CHECK-NEXT:                  | ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CHECK-NEXT: [[# @LINE + 3 ]] | void free_function_cannot_be_setter(int value);
// CHECK-NEXT:                  | ~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
GENPYBIND(setter_for("something"))
void free_function_cannot_be_setter(int value);

struct Something {
  // CHECK: methods.h:[[# @LINE + 2]]:3: error: Invalid annotation for constructor: getter_for("property")
  GENPYBIND(getter_for("property"))
  Something(int);

  // CHECK: methods.h:[[# @LINE + 2]]:3: error: Invalid annotation for constructor: setter_for("property")
  GENPYBIND(setter_for("property"))
  Something(double);

  // CHECK: methods.h:[[# @LINE + 3]]:3: error: Invalid annotation for conversion function: getter_for("property")
  // CHECK: methods.h:[[# @LINE + 2]]:3: error: Invalid annotation for conversion function: setter_for("property")
  GENPYBIND(getter_for("property"), setter_for("property"))
  operator bool() const;

  // CHECK: methods.h:[[# @LINE + 2]]:13: error: Invalid annotation for operator: getter_for("property")
  GENPYBIND(getter_for("property"))
  Something operator+() const;

  // CHECK: methods.h:[[# @LINE + 2]]:13: error: Invalid annotation for operator: setter_for("property")
  GENPYBIND(setter_for("property"))
  Something operator<(double) const;

  // CHECK: methods.h:[[# @LINE + 3]]:3: error: Invalid annotation for named declaration: getter_for("property")
  // CHECK: methods.h:[[# @LINE + 2]]:3: error: Invalid annotation for named declaration: setter_for("property")
  GENPYBIND(getter_for("property"), setter_for("property"))
  ~Something();
};

template <typename T> struct Example {};

// CHECK: methods.h:[[# @LINE + 3]]:1: error: Invalid annotation for named declaration: getter_for("property")
// CHECK: methods.h:[[# @LINE + 2]]:1: error: Invalid annotation for named declaration: setter_for("property")
GENPYBIND(getter_for("property"), setter_for("property"))
Example(float) -> Example<double>;

// CHECK: 12 errors generated.
