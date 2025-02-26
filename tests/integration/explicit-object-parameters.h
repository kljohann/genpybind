// SPDX-FileCopyrightText: 2025 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

// === Non-templated member functions

struct GENPYBIND(visible) Plain {
  int value = 0;

  void set_double(this Plain &self, int value);
  int get_double(this Plain const &self);
};

// === As mixin / replacement for CRTP

struct Base {
  template <typename Self> int get_value(this Self &&self) {
    return self.value;
  }
  template <typename Self> void set_value(this Self &&self, int value) {
    self.value = value;
  }
};

// `using` declaration + explicit instantiation: The definitions from base
// should be exposed, but only those with matching types.
struct GENPYBIND(visible) ExplInstUsing : public Base {
  int value = 0;

  using Base::get_value;
  using Base::set_value;
};
extern template int Base::get_value(const ExplInstUsing &);
extern template void Base::set_value(ExplInstUsing &, int value);

// `inline_base` + explicit instantiation: same as `using` + expl. inst.
struct GENPYBIND(inline_base) ExplInstInlined : public Base {
  int value = 0;
};
extern template int Base::get_value(const ExplInstInlined &);
extern template void Base::set_value(ExplInstInlined &, int value);

// `using` w/o explicit instantiation: Nothing should be exposed, as overloads
// with other types are not relevant.
struct GENPYBIND(visible) Using : public Base {
  int value = 0;

  using Base::get_value;
  using Base::set_value;
};

// `inline_base` w/o explicit instantiation: same as `using` w/o expl. inst.
// TODO: Should `inline_base` attempt instantiation in simple cases (e.g., when
// there are no other template arguments)?
struct GENPYBIND(inline_base) Inlined : public Base {
  int value = 0;
};

// === With visible base: methods are exposed as overloads on base itself.

struct GENPYBIND(visible) VisibleBase {
  template <typename Self> int to_kind(this Self &&self) { return self.kind; }
}; // OpBase

struct GENPYBIND(visible) One : VisibleBase {
  int kind = 1;
};
struct GENPYBIND(visible) Two : VisibleBase {
  int kind = 2;
};

extern template int VisibleBase::to_kind(this const One &self);
extern template int VisibleBase::to_kind(this const Two &self);
