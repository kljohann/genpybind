// RUN: genpybind-tool --xfail %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

// CHECK: arguments.h:[[# @LINE + 1]]:11: error: Wrong number of arguments for 'module' annotation
namespace example GENPYBIND(module("uiae", true)) {} // namespace )
// CHECK-NEXT: namespace example GENPYBIND(module("uiae", true)) {}
// CHECK-NEXT: ~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CHECK: arguments.h:[[# @LINE + 1]]:11: error: Wrong type of argument for 'module' annotation: 123
namespace example GENPYBIND(module(123)) {} // namespace )
// CHECK: 2 errors generated.
