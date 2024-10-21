// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

// CHECK: arguments.h:[[# @LINE + 1]]:55: error: Wrong number of arguments for 'visible' annotation
struct GENPYBIND(visible(true, "too many arguments")) X1 {};
// CHECK-NEXT: struct GENPYBIND(visible(true, "too many arguments")) X1 {};
// CHECK-NEXT: ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~
// CHECK: arguments.h:[[# @LINE + 1]]:41: error: Wrong type of argument for 'visible' annotation: "wrong type"
struct GENPYBIND(visible("wrong type")) X2 {};
// CHECK: arguments.h:[[# @LINE + 1]]:32: error: Wrong number of arguments for 'hidden' annotation
struct GENPYBIND(hidden(true)) X3 {};
// CHECK: 3 errors generated.
