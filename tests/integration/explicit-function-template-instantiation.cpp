// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "explicit-function-template-instantiation.h"

template <typename T> T identity(const T &value) { return value; }
template <int N> int constant() { return N; }
template <int N> int oops() { return N; }

template int identity<int>(const int &);
template double identity<double>(const double &);

template int constant<42>();

template int oops<123>();
template int oops<321>();
template int oops<42>();

template <typename T> T Example::plus_one(T value) { return value + 1; }

template int Example::plus_one(int);
template double Example::plus_one(double);

template <typename T>
TemplatedConstructor::TemplatedConstructor(T value) : value(value) {}

template TemplatedConstructor::TemplatedConstructor(int);

template <typename T> template <int N> T Tpl<T>::magic_value() const {
  return N;
}

template struct Tpl<int>;
template int Tpl<int>::magic_value<42>() const;
