// RUN: genpybind-tool %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

// CHECK: context.h:[[# @LINE + 1]]:8: warning: Declaration context contains 'visible' declarations but is not exposed
struct A {
  struct GENPYBIND(visible) B {};
};

// CHECK: 1 warning generated.
