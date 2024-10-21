// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

namespace deeply {
namespace nested {
struct GENPYBIND(visible, expose_as(NestedBase)) Base {};
} // namespace nested
} // namespace deeply

struct GENPYBIND(visible) Base {};
struct GENPYBIND(visible) Base2 {};

struct GENPYBIND(visible, hide_base) HideAll : deeply::nested::Base,
                                               Base,
                                               Base2 {};

struct GENPYBIND(visible, hide_base("Base")) HideUnqualified
    : deeply::nested::Base,
      Base,
      Base2 {};

struct GENPYBIND(visible, hide_base("::Base")) HideQualified
    : deeply::nested::Base,
      Base,
      Base2 {};

struct GENPYBIND(visible, hide_base("deeply::nested::Base")) HideQualifiedNested
    : deeply::nested::Base,
      Base,
      Base2 {};

struct GENPYBIND(visible, hide_base("::Base", "::Base2")) HideMultiple
    : deeply::nested::Base,
      Base,
      Base2 {};

template <typename T> struct GENPYBIND(visible) BaseTemplate {};

struct GENPYBIND(visible, hide_base("::BaseTemplate")) HideTemplate
    : Base,
      BaseTemplate<int>,
      BaseTemplate<bool> {};
