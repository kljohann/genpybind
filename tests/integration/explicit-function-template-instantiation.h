// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

/// \brief Expose instantiations of a function template.
template <typename T> T identity(const T &value) GENPYBIND(visible);

/// This always returns the same number.
template <int N> int constant() GENPYBIND(visible);

extern template int identity<int>(const int &);
extern template double identity<double>(const double &);

extern template int constant<42>();

/// It's not possible to call a specific instantiation from Python as it
/// does not accept arguments.
template <int N> int oops() GENPYBIND(visible);

extern template int oops<123>();
extern template int oops<321>();

/// But an instantiation can be given a distinct name.
extern template GENPYBIND(expose_as(oops_42)) int oops<42>();

struct GENPYBIND(visible) Example {
  template <typename T> T plus_one(T val);
};

extern template int Example::plus_one(int);
extern template double Example::plus_one(double);

struct GENPYBIND(visible) TemplatedConstructor {
  template <typename T> TemplatedConstructor(T value);

  int value = 0;
  int getValue() { return value; }
};

extern template TemplatedConstructor::TemplatedConstructor(int);

template <typename T> struct GENPYBIND(visible) Tpl {
  template <int N> T magic_value() const;
};

extern template struct Tpl<int>;
extern template int Tpl<int>::magic_value<42>() const;
