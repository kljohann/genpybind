// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

struct GENPYBIND(visible) Target {};

// CHECK: annotations.h:[[# @LINE + 1]]:42: error: Invalid annotation for class: export_values()
struct GENPYBIND(visible, export_values) Context {
  // CHECK: annotations.h:[[# @LINE + 1]]:9: error: Invalid annotation for type alias: getter_for("something")
  using alias GENPYBIND(getter_for(something)) = Target;
};

// CHECK: annotations.h:[[# @LINE + 2]]:18: error: Invalid annotation for variable: manual()
// CHECK: annotations.h:[[# @LINE + 1]]:18: error: Invalid annotation for variable: postamble()
extern const int global_const_variable GENPYBIND(manual, postamble);

struct GENPYBIND(visible) Example {
  // CHECK: annotations.h:[[# @LINE + 2]]:3: error: 'postamble' can only be used in global scope
  GENPYBIND(postamble)
  GENPYBIND_MANUAL({ (void)parent; });

  // CHECK: annotations.h:[[# @LINE + 1]]:54: error: 'postamble' can only be used in global scope
  static constexpr auto GENPYBIND(manual, postamble) almost_valid =
      [](auto &context) { (void)context; };

  // CHECK: annotations.h:[[# @LINE + 3]]:24: error: Invalid annotation for variable: manual()
  // CHECK: annotations.h:[[# @LINE + 2]]:24: error: Invalid annotation for variable: postamble()
  GENPYBIND(manual, postamble)
  static constexpr int static_constexpr_variable = 3;

  // CHECK: annotations.h:[[# @LINE + 3]]:7: error: Invalid annotation for variable: manual()
  // CHECK: annotations.h:[[# @LINE + 2]]:7: error: Invalid annotation for variable: postamble()
  GENPYBIND(manual, postamble)
  int field = 1;

  // CHECK: annotations.h:[[# @LINE + 2]]:54: error: Invalid annotation for variable: manual()
  // CHECK: annotations.h:[[# @LINE + 1]]:54: error: Invalid annotation for variable: postamble()
  static constexpr auto GENPYBIND(manual, postamble) invalid = []() {};

  // CHECK: annotations.h:[[# @LINE + 2]]:54: error: Invalid annotation for variable: manual()
  // CHECK: annotations.h:[[# @LINE + 1]]:54: error: Invalid annotation for variable: postamble()
  static constexpr auto GENPYBIND(manual, postamble) also_invalid =
      [](int value) { (void)value; };

  // CHECK: annotations.h:[[# @LINE + 2]]:54: error: Invalid annotation for variable: manual()
  // CHECK: annotations.h:[[# @LINE + 1]]:54: error: Invalid annotation for variable: postamble()
  static constexpr auto GENPYBIND(manual, postamble) invalid_but_close =
      [](auto value) { (void)value; };
};

// CHECK: 16 errors generated.
