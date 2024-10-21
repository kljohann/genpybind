// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

struct Example {
  // CHECK: constructor.h:[[# @LINE + 1]]:3: error: Invalid annotation for non-converting constructor: implicit_conversion
  Example() GENPYBIND(implicit_conversion);

  Example(int value) GENPYBIND(implicit_conversion);

  // CHECK: constructor.h:[[# @LINE + 1]]:3: error: Invalid annotation for non-converting constructor: implicit_conversion
  Example(int first, int second) GENPYBIND(implicit_conversion);
};

// CHECK: 2 errors generated.
