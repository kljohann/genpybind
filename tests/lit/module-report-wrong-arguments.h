// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

// CHECK: arguments.h:[[# @LINE + 1]]:11: error: Wrong number of arguments for 'module' annotation
namespace example GENPYBIND(module("uiae", true)) {} // namespace )
// CHECK-NEXT: namespace example GENPYBIND(module("uiae", true)) {}
// CHECK-NEXT: ~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CHECK: arguments.h:[[# @LINE + 1]]:11: error: Wrong type of argument for 'module' annotation: 123
namespace example_2 GENPYBIND(module(123)) {} // namespace )
// CHECK: 2 errors generated.
