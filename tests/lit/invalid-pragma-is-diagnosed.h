// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

// clang-format off
// CHECK:      diagnosed.h:[[# @LINE + 3]]:26: error: expected "FILENAME" or <FILENAME>
// CHECK-NEXT: #pragma genpybind include
// CHECK-NEXT:                          ^
#pragma genpybind include
// CHECK:      diagnosed.h:[[# @LINE + 3]]:27: error: expected "FILENAME" or <FILENAME>
// CHECK-NEXT: #pragma genpybind include UIAE
// CHECK-NEXT:                           ^
#pragma genpybind include UIAE
#define SOMETHING <genpybind/genpybind.h>
// CHECK:      diagnosed.h:[[# @LINE + 3]]:27: error: expected "FILENAME" or <FILENAME>
// CHECK-NEXT: #pragma genpybind include SOMETHING
// CHECK-NEXT:                           ^
#pragma genpybind include SOMETHING
#define SPURIOUS
// CHECK:      diagnosed.h:[[# @LINE + 3]]:51: error: expected end of line in preprocessor expression
// CHECK-NEXT: #pragma genpybind include <genpybind/genpybind.h> SPURIOUS
// CHECK-NEXT:                                                   ^
#pragma genpybind include <genpybind/genpybind.h> SPURIOUS
// CHECK:      diagnosed.h:[[# @LINE + 3]]:18: error: invalid preprocessing directive
// CHECK-NEXT: #pragma genpybind
// CHECK-NEXT:                  ^
#pragma genpybind
// CHECK:      diagnosed.h:[[# @LINE + 3]]:19: error: invalid preprocessing directive
// CHECK-NEXT: #pragma genpybind uiae
// CHECK-NEXT:                   ^
#pragma genpybind uiae
// CHECK:      diagnosed.h:[[# @LINE + 3]]:19: error: invalid preprocessing directive
// CHECK-NEXT: #pragma genpybind define XYZ 123
// CHECK-NEXT:                   ^
#pragma genpybind define XYZ 123
// CHECK:      diagnosed.h:[[# @LINE + 4]]:18: error: invalid preprocessing directive
// CHECK-NEXT: #pragma genpybind
// CHECK-NEXT:                  ^
// CHECK: 8 errors generated.
#pragma genpybind
// clang-format on
