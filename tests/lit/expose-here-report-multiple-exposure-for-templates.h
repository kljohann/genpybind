// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

template <typename T> struct Something {
  enum class Enum { A, B, C };
};

using Usage GENPYBIND(expose_here) = Something<int>;

struct B {
  // CHECK: templates.h:[[# @LINE + 6]]:9: error: 'Something' has already been exposed elsewhere
  // CHECK-NEXT: using Duplicate GENPYBIND(expose_here) = Something<int>;
  // CHECK-NEXT: ~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // CHECK-NEXT: templates.h:[[# @LINE - 6]]:7: note: Previously exposed here
  // CHECK-NEXT: using Usage GENPYBIND(expose_here) = Something<int>;
  // CHECK-NEXT: ~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  using Duplicate GENPYBIND(expose_here) = Something<int>;
};

// CHECK: 1 error generated.
