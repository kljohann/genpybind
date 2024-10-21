// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

// CHECK: arguments.h:[[# @LINE + 1]]:54: error: Wrong number of arguments for 'expose_as' annotation
struct GENPYBIND(expose_as("too_many", "arguments")) X1 {};
// CHECK: arguments.h:[[# @LINE + 1]]:35: error: Wrong type of argument for 'expose_as' annotation: true
struct GENPYBIND(expose_as(true)) X2 {};
// CHECK: arguments.h:[[# @LINE + 1]]:49: error: Invalid spelling in 'expose_as' annotation: 'invalid spelling'
struct GENPYBIND(expose_as("invalid spelling")) X3 {};
// CHECK-NEXT: struct GENPYBIND(expose_as("invalid spelling")) X3 {};
// CHECK-NEXT: ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~
// CHECK: arguments.h:[[# @LINE + 1]]:33: error: Invalid spelling in 'expose_as' annotation: ''
struct GENPYBIND(expose_as("")) X4 {};
// CHECK: 4 errors generated.
