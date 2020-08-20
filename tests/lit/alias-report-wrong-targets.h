// RUN: genpybind-tool --xfail %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

// CHECK: wrong-targets.h:[[# @LINE + 1]]:7: error: Annotated alias has unsupported target kind
using something GENPYBIND(expose_here) = bool;

// CHECK: wrong-targets.h:[[# @LINE + 1]]:7: error: Annotated alias has unsupported target kind
using something_else GENPYBIND(visible) = int;

// CHECK: wrong-targets.h:[[# @LINE + 1]]:7: error: Annotated alias has unsupported target kind
using function_type GENPYBIND(expose_here) = int (*)(int, bool);

// CHECK: wrong-targets.h:[[# @LINE + 1]]:7: error: Annotated alias has unsupported target kind
using function_type_alias GENPYBIND(visible) = int (*)(int, bool);

// CHECK: wrong-targets.h:[[# @LINE + 1]]:13: error: Annotated alias has unsupported target kind
typedef int FunctionType() const GENPYBIND(expose_here);

// CHECK: 5 errors generated.
