// RUN: genpybind-tool %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

struct A {
  // CHECK: context.h:[[# @LINE + 1]]:29: warning: Nested declaration context is 'visible' but unreachable
  struct GENPYBIND(visible) B {};
};

// CHECK: 1 warning generated.
