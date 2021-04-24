// RUN: genpybind-tool --xfail %s -- 2>&1 \
// RUN: | FileCheck %s --strict-whitespace
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
