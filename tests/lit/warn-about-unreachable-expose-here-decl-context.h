// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

template <typename T> struct Type {
  struct GENPYBIND(visible) Something {};
};

// CHECK: context.h:[[# @LINE + 3]]:8: warning: Declaration context 'A' contains 'visible' declarations but is not exposed
// CHECK-NEXT: struct A {
// CHECK-NEXT: ~~~~~~~^~~
struct A {
  // CHECK: context.h:[[# @LINE + 3]]:9: warning: Declaration context 'A::Instantiation' contains 'visible' declarations but is not exposed
  // CHECK-NEXT: using Instantiation GENPYBIND(expose_here) = Type<int>;
  // CHECK-NEXT: ~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  using Instantiation GENPYBIND(expose_here) = Type<int>;
};

// CHECK: 2 warnings generated.
