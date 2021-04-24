// RUN: genpybind-tool %s -- 2>&1 \
// RUN: | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

template <typename T> struct Type {
  struct GENPYBIND(visible) Something {};
};

// CHECK: qualified-name.h:[[# @LINE + 3]]:23: warning: Declaration context 'Type<bool>' contains 'visible' declarations but is not exposed
// CHECK-NEXT: extern template class Type<bool>;
// CHECK-NEXT: ~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~
extern template class Type<bool>;

// TODO: Is the following warning desirable?

template <typename T>
// CHECK: qualified-name.h:[[# @LINE + 3]]:8: warning: Declaration context 'Other<B>' contains 'visible' declarations but is not exposed
// CHECK-NEXT: struct Other {
// CHECK-NEXT: ~~~~~~~^~~~~~~
struct Other {
  struct GENPYBIND(visible) Something {};
  using Self GENPYBIND(visible) = Other<T>;
};

struct B : public Other<B> {};

// CHECK: 2 warnings generated.
