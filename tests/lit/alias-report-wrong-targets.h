// RUN: genpybind-tool --xfail %s -- 2>&1 \
// RUN: | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

// CHECK: wrong-targets.h:[[# @LINE + 2]]:7: error: Annotated alias has unsupported target kind
// CHECK: wrong-targets.h:[[# @LINE + 1]]:7: error: Invalid annotation for named declaration: expose_here()
using something GENPYBIND(expose_here) = bool;

// CHECK: wrong-targets.h:[[# @LINE + 1]]:7: error: Annotated alias has unsupported target kind
using something_else GENPYBIND(visible) = int;

// CHECK: wrong-targets.h:[[# @LINE + 2]]:7: error: Annotated alias has unsupported target kind
// CHECK: wrong-targets.h:[[# @LINE + 1]]:7: error: Invalid annotation for named declaration: expose_here()
using function_type GENPYBIND(expose_here) = int (*)(int, bool);

// CHECK: wrong-targets.h:[[# @LINE + 1]]:7: error: Annotated alias has unsupported target kind
using function_type_alias GENPYBIND(visible) = int (*)(int, bool);

// CHECK: wrong-targets.h:[[# @LINE + 2]]:13: error: Annotated alias has unsupported target kind
// CHECK: wrong-targets.h:[[# @LINE + 1]]:13: error: Invalid annotation for named declaration: expose_here()
typedef int FunctionType() const GENPYBIND(expose_here);

// CHECK: 8 errors generated.
