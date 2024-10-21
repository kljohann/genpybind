// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

struct GENPYBIND(visible) Target {};

typedef Target typedef_unannotated;
typedef Target typedef_visible GENPYBIND(visible);
typedef Target typedef_defaulted GENPYBIND(visible(default));
typedef Target typedef_hidden GENPYBIND(hidden);

using using_unannotated = Target;
using using_visible GENPYBIND(visible) = Target;
using using_defaulted GENPYBIND(visible(default)) = Target;
using using_hidden GENPYBIND(hidden) = Target;

struct GENPYBIND(visible) VisibleParent {
  typedef Target typedef_unannotated;
  typedef Target typedef_visible GENPYBIND(visible);
  typedef Target typedef_defaulted GENPYBIND(visible(default));
  typedef Target typedef_hidden GENPYBIND(hidden);

  using using_unannotated = Target;
  using using_visible GENPYBIND(visible) = Target;
  using using_defaulted GENPYBIND(visible(default)) = Target;
  using using_hidden GENPYBIND(hidden) = Target;
};

struct UnexposedTarget {};
typedef UnexposedTarget typedef_unexposed GENPYBIND(visible);
using using_unexposed GENPYBIND(visible) = UnexposedTarget;

struct ForwardDeclaredTarget;
typedef ForwardDeclaredTarget typedef_forward_declared GENPYBIND(visible);
using using_forward_declared GENPYBIND(visible) = ForwardDeclaredTarget;
struct GENPYBIND(visible) ForwardDeclaredTarget {};
