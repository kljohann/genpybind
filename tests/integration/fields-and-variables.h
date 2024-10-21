// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

extern int global_variable GENPYBIND(visible);
extern const int global_const_variable GENPYBIND(visible);

struct GENPYBIND(visible) Example {
  static int static_variable;
  static const int static_const_variable = 2;
  static constexpr int static_constexpr_variable = 3;

  GENPYBIND(readonly)
  static int readonly_static_variable;
  GENPYBIND(readonly)
  static const int readonly_static_const_variable = 2;

  int field = 1;
  const int const_field = 2;
  GENPYBIND(readonly)
  int readonly_field = 3;
  GENPYBIND(readonly)
  const int readonly_const_field = 4;
};
