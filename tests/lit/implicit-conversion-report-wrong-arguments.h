// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

struct Example {
  /*implicit*/ Example(int value)
      GENPYBIND(implicit_conversion, implicit_conversion(false),
                implicit_conversion(true));

  // CHECK: arguments.h:[[# @LINE + 1]]:16: error: Wrong number of arguments for 'implicit_conversion' annotation
  /*implicit*/ Example(bool value) GENPYBIND(implicit_conversion(true, false));

  // CHECK: arguments.h:[[# @LINE + 3]]:16: error: Wrong type of argument for 'implicit_conversion' annotation: default
  // CHECK: arguments.h:[[# @LINE + 2]]:16: error: Wrong type of argument for 'implicit_conversion' annotation: 123
  // CHECK: arguments.h:[[# @LINE + 1]]:16: error: Wrong type of argument for 'implicit_conversion' annotation: "uiae"
  /*implicit*/ Example(float value)
      GENPYBIND(implicit_conversion(default), implicit_conversion(123),
                implicit_conversion("uiae"));
};

// CHECK: 4 errors generated.
