// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

struct GENPYBIND(visible) Aggregate {
  int a;
  int b;
  int c;
};

struct GENPYBIND(visible) WithDefaultInitializers {
  int lucky_number = 5;
  bool it_just_works = true;
};

struct GENPYBIND(visible) CamelCase {
  int CamelField;
};

struct GENPYBIND(visible) WithBases : Aggregate, CamelCase {
  int d;
};

struct GENPYBIND(inline_base(Aggregate)) WithInlinedBase : Aggregate {
  int d;
};

struct GENPYBIND(hide_base(Aggregate)) WithHiddenBase : Aggregate {
  int d;
};
