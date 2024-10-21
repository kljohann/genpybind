// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

struct GENPYBIND(visible) Base {
  int m_public = 123;

protected:
  using some_type = int;
  int m_protected = -123;
};

struct GENPYBIND(visible) Derived : Base {
  using Base::some_type;
  // TODO: Is exposed via &Base::m_protected, instead of Derived::m_protected.
  // using Base::m_protected;

private:
  // TODO: m_public is still accessible in Python via base
  using Base::m_public;
};

struct GENPYBIND(visible, hide_base("Base")) DerivedHidden : Base {
  using Base::m_public;
  // TODO: Is exposed via &Base::m_protected.
  // using Base::m_protected;
};

struct GENPYBIND(visible, inline_base("Base")) DerivedInlined : Base {
  // TODO: Is exposed via &Base::m_protected.
  // using Base::m_protected;

private:
  using Base::m_public;
};

struct GENPYBIND(visible, hide_base("Base")) DerivedAnnotated : Base {
  // TODO: Annotations are not taken into account
  using Base::m_public GENPYBIND(expose_as(public));
  // TODO: Is exposed via &Base::m_protected.
  // using Base::m_protected GENPYBIND(expose_as(protected));
};

struct GENPYBIND(hidden) CtorBase {
  CtorBase(int value_) : value(value_) {}
  int value;
};

struct GENPYBIND(visible) Ctor : CtorBase {
  // TODO: Also pulls in implicit copy constructor
  // using CtorBase::CtorBase;
  using CtorBase::value;
};

struct GENPYBIND(visible) TplBase {
  template <typename T> T neg(T value);
};

extern template int TplBase::neg(int);

struct GENPYBIND(visible, hide_base("TplBase")) Tpl : TplBase {
  using TplBase::neg;
};
