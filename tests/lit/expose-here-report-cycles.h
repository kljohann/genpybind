// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

struct Two;

struct One {
  // CHECK: cycles.h:[[# @LINE + 1]]:9: error: 'expose_here' annotations form a cycle
  using Alias GENPYBIND(expose_here) = Two;
};

struct Two {
  // CHECK: cycles.h:[[# @LINE + 1]]:9: error: 'expose_here' annotations form a cycle
  using Alias GENPYBIND(expose_here) = One;
};

struct Three {
  // CHECK: cycles.h:[[# @LINE + 1]]:9: error: 'expose_here' annotations form a cycle
  using Alias GENPYBIND(expose_here) = Three;
};

struct Via1;
struct Via2;
struct Via3;

struct Via1 {
  // CHECK: cycles.h:[[# @LINE + 1]]:9: error: 'expose_here' annotations form a cycle
  using Alias GENPYBIND(expose_here) = Via2;
};

struct Via2 {
  // CHECK: cycles.h:[[# @LINE + 1]]:9: error: 'expose_here' annotations form a cycle
  using Alias GENPYBIND(expose_here) = Via3;
};

struct Via3 {
  // CHECK: cycles.h:[[# @LINE + 1]]:9: error: 'expose_here' annotations form a cycle
  using Alias GENPYBIND(expose_here) = Via1;
};

// CHECK: 6 errors generated.
