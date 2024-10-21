// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

GENPYBIND(noconvert(argument))
void example(double argument, bool other);

// CHECK: specifiers.h:[[# @LINE + 6 ]]:6: error: Invalid argument specifier in 'noconvert' annotation: 'oops'
// CHECK-NEXT: [[# @LINE + 4 ]] | GENPYBIND(noconvert(oops))
// CHECK-NEXT:                  | ~~~~~~~~~~~~~~~~~~~~~~~~~~
// CHECK-NEXT: [[# @LINE + 3 ]] | void something(double argument, bool other);
// CHECK-NEXT:                  | ~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
GENPYBIND(noconvert(oops))
void something(double argument, bool other);

struct GENPYBIND(visible) Other {};

struct GENPYBIND(visible) Parent {
  GENPYBIND(keep_alive(this, other))
  void something(Other *other);

  GENPYBIND(keep_alive(this, return))
  Other *something();

  // CHECK: specifiers.h:[[# @LINE + 2]]:8: error: Invalid argument specifier in 'keep_alive' annotation: 'other'
  GENPYBIND(keep_alive(this, other))
  void something(int);

  GENPYBIND(return_value_policy(copy))
  Other &as_copy();

  // CHECK: specifiers.h:[[# @LINE + 2]]:8: error: Wrong number of arguments for 'return_value_policy' annotation
  GENPYBIND(return_value_policy)
  void oops();

  // CHECK: specifiers.h:[[# @LINE + 2]]:8: error: Wrong number of arguments for 'return_value_policy' annotation
  GENPYBIND(return_value_policy(copy, reference_internal))
  void oops(int);

  // CHECK: specifiers.h:[[# @LINE + 2]]:8: error: Wrong number of arguments for 'keep_alive' annotation
  GENPYBIND(keep_alive)
  void oops(float);

  // CHECK: specifiers.h:[[# @LINE + 2]]:8: error: Wrong number of arguments for 'keep_alive' annotation
  GENPYBIND(keep_alive(this, first, second))
  void oops(Other *first, Other *second);
};

GENPYBIND(keep_alive(parent, other), required(other), required(other, parent),
          noconvert(value), noconvert(parent, other))
void correct_number_of_arguments(Parent *parent, Other *other, double value);

// CHECK: specifiers.h:[[# @LINE + 2]]:6: error: Invalid argument specifier in 'keep_alive' annotation: 'this'
GENPYBIND(keep_alive(this, other))
void free_function_cannot_use_this(Other *other);

// CHECK: specifiers.h:[[# @LINE + 2]]:6: error: Wrong number of arguments for 'required' annotation
GENPYBIND(required)
void required_needs_argument(Other *other);

// CHECK: specifiers.h:[[# @LINE + 2]]:6: error: Wrong number of arguments for 'noconvert' annotation
GENPYBIND(noconvert)
void noconvert_needs_argument(double value);

// CHECK: 9 errors generated.
