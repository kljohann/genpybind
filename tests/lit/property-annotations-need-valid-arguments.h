// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

struct Example {
  GENPYBIND(getter_for(":rating!"), getter_for(""), getter_for(true),
            getter_for(default), getter_for(123))
  // CHECK: arguments.h:[[# @LINE + 5]]:7: error: Invalid spelling in 'getter_for' annotation: ':rating!'
  // CHECK: arguments.h:[[# @LINE + 4]]:7: error: Invalid spelling in 'getter_for' annotation: ''
  // CHECK: arguments.h:[[# @LINE + 3]]:7: error: Wrong type of argument for 'getter_for' annotation: true
  // CHECK: arguments.h:[[# @LINE + 2]]:7: error: Wrong type of argument for 'getter_for' annotation: default
  // CHECK: arguments.h:[[# @LINE + 1]]:7: error: Wrong type of argument for 'getter_for' annotation: 123
  int getRating() const;

  GENPYBIND(setter_for(":rating!"), setter_for(""), setter_for(true),
            setter_for(default), setter_for(123))
  // CHECK: arguments.h:[[# @LINE + 5]]:8: error: Invalid spelling in 'setter_for' annotation: ':rating!'
  // CHECK: arguments.h:[[# @LINE + 4]]:8: error: Invalid spelling in 'setter_for' annotation: ''
  // CHECK: arguments.h:[[# @LINE + 3]]:8: error: Wrong type of argument for 'setter_for' annotation: true
  // CHECK: arguments.h:[[# @LINE + 2]]:8: error: Wrong type of argument for 'setter_for' annotation: default
  // CHECK: arguments.h:[[# @LINE + 1]]:8: error: Wrong type of argument for 'setter_for' annotation: 123
  void setRating(int value);

  GENPYBIND(getter_for("multiple", "properties"))
  int getter() const;

  GENPYBIND(getter_for)
  // CHECK: arguments.h:[[# @LINE + 1]]:7: error: Wrong number of arguments for 'getter_for' annotation
  int getSize() const;

  GENPYBIND(setter_for("multiple", "properties"))
  void setter(int value);

  GENPYBIND(setter_for)
  // CHECK: arguments.h:[[# @LINE + 1]]:8: error: Wrong number of arguments for 'setter_for' annotation
  void setSize(int value);
};

// CHECK: 12 errors generated.
