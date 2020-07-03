// RUN: genpybind-tool %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

template <typename T> struct Type {
  struct GENPYBIND(visible) Something {};
};

// TODO: These errors should be reported in the right order.

// CHECK-DAG: context.h:[[# @LINE + 3]]:8: warning: Declaration context contains 'visible' declarations but is not exposed
// CHECK-DAG: struct A {
// CHECK-DAG: ~~~~~~~^~~
struct A {
  // CHECK-DAG: context.h:[[# @LINE + 3]]:9: warning: Declaration context contains 'visible' declarations but is not exposed
  // CHECK-DAG: using Instantiation GENPYBIND(expose_here) = Type<int>;
  // CHECK-DAG: ~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  using Instantiation GENPYBIND(expose_here) = Type<int>;
};

// CHECK: 2 warnings generated.
