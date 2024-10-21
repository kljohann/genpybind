// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

struct GENPYBIND(visible) Target {};

// CHECK: parsing-errors.h:[[# @LINE + 1]]:8: error: Invalid genpybind annotation: something
struct GENPYBIND(visible, something) Context {
  // CHECK: parsing-errors.h:[[# @LINE + 1]]:15: error: Invalid token in genpybind annotation: 123
  using alias GENPYBIND(visible 123) = Target;
};

// CHECK: parsing-errors.h:[[# @LINE + 1]]:8: error: Invalid token in genpybind annotation while looking for ')': "abc"
struct GENPYBIND(expose_as(uiae "abc")) Xyz {};
// CHECK: 3 errors generated.
