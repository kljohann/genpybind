// RUN: genpybind-tool --xfail %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

// CHECK:      cycle.h:[[# @LINE + 3]]:8: error: 'expose_here' annotations form a cycle
// CHECK-NEXT: struct Base {};
// CHECK-NEXT: ~~~~~~~^~~~~~~
struct Base {};

struct GENPYBIND(visible) Derived : public Base {
  using Cycle GENPYBIND(expose_here) = Base;
};
// CHECK: 1 error generated.
