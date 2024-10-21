// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --match-full-lines

#pragma once

#include <genpybind/genpybind.h>

template <typename T> struct Type {
  struct GENPYBIND(visible) Something {};
};

// CHECK: {{.*}}qualified-name.h:[[# @LINE + 3 ]]:23: warning: Declaration context 'Type<bool>' contains 'visible' declarations but is not exposed
// CHECK-NEXT: {{[0-9]*}} | extern template class Type<bool>;
// CHECK-NEXT: {{[0-9]*}} | ~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~
extern template class Type<bool>;

// TODO: Is the following warning desirable?

template <typename T>
// CHECK: {{.*}}qualified-name.h:[[# @LINE + 4 ]]:8: warning: Declaration context 'Other<B>' contains 'visible' declarations but is not exposed
// CHECK-NEXT: {{[0-9]*}} | template <typename T>
//      CHECK: {{[0-9]*}} | struct Other {
// CHECK-NEXT: {{[0-9]*}} | ~~~~~~~^~~~~~~
struct Other {
  struct GENPYBIND(visible) Something {};
  using Self GENPYBIND(visible) = Other<T>;
};

struct B : public Other<B> {};

// CHECK: 2 warnings generated.
