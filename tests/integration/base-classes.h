// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

struct Derived;

// Even if this declaration is processed earlier, the base class has to be
// exposed first in order to avoid "referenced unknown base" errors.
using Early GENPYBIND(expose_here, expose_as(Derived)) = Derived;

namespace deeply {
namespace nested {
struct GENPYBIND(visible) Base {};

template <typename T> struct GENPYBIND(visible) CRTP {};

struct Hidden {};
} // namespace nested
} // namespace deeply

struct GENPYBIND(visible) Derived : public deeply::nested::Base {};

struct GENPYBIND(visible) DerivedCRTP
    : public deeply::nested::CRTP<DerivedCRTP> {};

struct GENPYBIND(visible) DerivedPrivate : private deeply::nested::Base {};

struct GENPYBIND(visible) DerivedProtected : protected deeply::nested::Base {};

struct GENPYBIND(visible) DerivedFromHidden : public deeply::nested::Hidden {};

struct GENPYBIND(visible) Abstract {
  virtual ~Abstract() = default;

  static bool static_method();

  virtual int abstract() const = 0;
  virtual int defined_in_base() const;
  virtual bool overridden() const;
};

struct GENPYBIND(visible) DerivedFromAbstract : public Abstract {
  int abstract() const override;
  bool overridden() const override;
};
