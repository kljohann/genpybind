// RUN: genpybind-tool --xfail %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

// CHECK: arguments.h:[[# @LINE + 1]]:55: error: Wrong number of arguments for 'visible' annotation
struct GENPYBIND(visible(true, "too many arguments")) X1 {};
// CHECK-NEXT: struct GENPYBIND(visible(true, "too many arguments")) X1 {};
// CHECK-NEXT: ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~
// CHECK: arguments.h:[[# @LINE + 1]]:41: error: Wrong type of argument for 'visible' annotation: "wrong type"
struct GENPYBIND(visible("wrong type")) X2 {};
// CHECK: arguments.h:[[# @LINE + 1]]:32: error: Wrong number of arguments for 'hidden' annotation
struct GENPYBIND(hidden(true)) X3 {};
// CHECK: 3 errors generated.
