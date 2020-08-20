// RUN: genpybind-tool --xfail %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

struct GENPYBIND(visible) Target {};

// CHECK: annotations.h:[[# @LINE + 1]]:42: error: Invalid annotation for record: export_values()
struct GENPYBIND(visible, export_values) Context {
  // CHECK: annotations.h:[[# @LINE + 1]]:9: error: Invalid annotation for type alias: getter_for("something")
  using alias GENPYBIND(getter_for(something)) = Target;
};
// CHECK: 2 errors generated.
