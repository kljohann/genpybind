// RUN: genpybind-tool --xfail %s -- 2>&1 \
// RUN: | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

namespace pybind11 {
class args;
class kwargs;
} // namespace pybind11

// CHECK: parameters.h:[[# @LINE + 2]]:48: error: cannot be followed by other parameters
GENPYBIND(visible)
void foo(pybind11::args args, pybind11::kwargs kwargs, bool oops);
// CHECK-NEXT: void foo(pybind11::args args, pybind11::kwargs kwargs, bool oops);
// CHECK-NEXT:                               ~~~~~~~~~~~~~~~~~^~~~~~

// CHECK: 1 error generated.
