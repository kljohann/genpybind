// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

namespace pybind11 {
class args;
class kwargs;
} // namespace pybind11

// This is fine: `args` implies `kw_only` for all remaining arguments.
GENPYBIND(visible)
void foo(const pybind11::args &args, bool oops);

// CHECK: parameters.h:[[# @LINE + 2]]:48: error: cannot be followed by other parameters
GENPYBIND(visible)
void foo(pybind11::args args, pybind11::kwargs kwargs, bool oops);
// CHECK-NEXT: void foo(pybind11::args args, pybind11::kwargs kwargs, bool oops);
// CHECK-NEXT:                               ~~~~~~~~~~~~~~~~~^~~~~~

// CHECK: 1 error generated.
