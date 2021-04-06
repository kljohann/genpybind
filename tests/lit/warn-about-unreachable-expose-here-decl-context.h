// RUN: genpybind-tool %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

template <typename T> struct Type {
  struct GENPYBIND(visible) Something {};
};

// CHECK: context.h:[[# @LINE + 3]]:8: warning: Declaration context 'A' contains 'visible' declarations but is not exposed
// CHECK-NEXT: struct A {
// CHECK-NEXT: ~~~~~~~^~~
struct A {
  // CHECK: context.h:[[# @LINE + 3]]:9: warning: Declaration context 'A::Instantiation' contains 'visible' declarations but is not exposed
  // CHECK-NEXT: using Instantiation GENPYBIND(expose_here) = Type<int>;
  // CHECK-NEXT: ~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  using Instantiation GENPYBIND(expose_here) = Type<int>;
};

// CHECK: 2 warnings generated.
