// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

struct GENPYBIND(visible) Example {
  int getter_defined_twice() GENPYBIND(getter_for("getter_twice"));
  // CHECK: setter.h:[[# @LINE + 1]]:7: error: getter already defined for 'getter_twice'
  int getter_defined_twice_2() GENPYBIND(getter_for("getter_twice"));

  int setter_defined_twice() GENPYBIND(getter_for("setter_twice"));
  void setter_defined_twice(int value) GENPYBIND(setter_for("setter_twice"));
  // CHECK: setter.h:[[# @LINE + 1]]:8: error: setter already defined for 'setter_twice'
  void setter_defined_twice_2(int value) GENPYBIND(setter_for("setter_twice"));

  int both_defined_twice() GENPYBIND(getter_for("both_twice"));
  // CHECK: setter.h:[[# @LINE + 1]]:7: error: getter already defined for 'both_twice'
  int both_defined_twice_2() GENPYBIND(getter_for("both_twice"));
  void both_defined_twice(int value) GENPYBIND(setter_for("both_twice"));
  // CHECK: setter.h:[[# @LINE + 1]]:8: error: setter already defined for 'both_twice'
  void both_defined_twice_2(int value) GENPYBIND(setter_for("both_twice"));

  // CHECK: setter.h:[[# @LINE + 1]]:8: error: No getter for the 'prop' property
  void setter_without_getter(int value) GENPYBIND(setter_for("prop"));
};

// CHECK: 5 errors generated.
