// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

// CHECK: context.h:[[# @LINE + 3]]:8: warning: Declaration context 'A' contains 'visible' declarations but is not exposed
// CHECK-NEXT: struct A {
// CHECK-NEXT: ~~~~~~~^~~
struct A {
  struct GENPYBIND(visible) B {};
};

// CHECK: 1 warning generated.
