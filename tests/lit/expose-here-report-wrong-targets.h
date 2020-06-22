// RUN: genpybind-tool --xfail %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

// CHECK: wrong-targets.h:[[# @LINE + 1]]:7: error: Unsupported target used with 'expose_here' annotation
using something GENPYBIND(expose_here) = bool;

// CHECK: wrong-targets.h:[[# @LINE + 1]]:7: error: Unsupported target used with 'expose_here' annotation
using function_type GENPYBIND(expose_here) = int (*)(int, bool);

// CHECK: wrong-targets.h:[[# @LINE + 1]]:13: error: Unsupported target used with 'expose_here' annotation
typedef int FunctionType() const GENPYBIND(expose_here);

// CHECK: 3 errors generated.
