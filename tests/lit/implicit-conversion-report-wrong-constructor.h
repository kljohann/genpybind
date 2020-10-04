// RUN: genpybind-tool --xfail %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

struct Example {
  // CHECK: constructor.h:[[# @LINE + 1]]:3: error: Invalid annotation for non-converting constructor: implicit_conversion
  Example() GENPYBIND(implicit_conversion);

  Example(int value) GENPYBIND(implicit_conversion);

  // CHECK: constructor.h:[[# @LINE + 1]]:3: error: Invalid annotation for non-converting constructor: implicit_conversion
  Example(int first, int second) GENPYBIND(implicit_conversion);
};

// CHECK: 2 errors generated.
