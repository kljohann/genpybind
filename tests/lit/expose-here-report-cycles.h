// RUN: genpybind-tool --xfail %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

struct Two;

// TODO: These errors should be reported in the right order.

struct One {
  // CHECK-DAG: cycles.h:[[# @LINE + 1]]:9: error: 'expose_here' annotations form a cycle
  using Alias GENPYBIND(expose_here) = Two;
};

struct Two {
  // CHECK-DAG: cycles.h:[[# @LINE + 1]]:9: error: 'expose_here' annotations form a cycle
  using Alias GENPYBIND(expose_here) = One;
};

struct Three {
  // CHECK-DAG: cycles.h:[[# @LINE + 1]]:9: error: 'expose_here' annotations form a cycle
  using Alias GENPYBIND(expose_here) = Three;
};

struct Via1;
struct Via2;
struct Via3;

struct Via1 {
  // CHECK-DAG: cycles.h:[[# @LINE + 1]]:9: error: 'expose_here' annotations form a cycle
  using Alias GENPYBIND(expose_here) = Via2;
};

struct Via2 {
  // CHECK-DAG: cycles.h:[[# @LINE + 1]]:9: error: 'expose_here' annotations form a cycle
  using Alias GENPYBIND(expose_here) = Via3;
};

struct Via3 {
  // CHECK-DAG: cycles.h:[[# @LINE + 1]]:9: error: 'expose_here' annotations form a cycle
  using Alias GENPYBIND(expose_here) = Via1;
};

// CHECK: 6 errors generated.
