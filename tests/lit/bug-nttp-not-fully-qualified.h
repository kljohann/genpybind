// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace
// XFAIL: *

#pragma once

// TODO: Non-type template parameters are not properly expanded to a fully
// qualified form.
// CHECK-NOT: Template<Abc::N>
// CHECK: ::nested::Template<::nested::Abc::N>

#include <genpybind/genpybind.h>

namespace nested {
template <int N> struct Template {};

struct Abc {
  static constexpr int N = 123;
};

void example(Template<Abc::N>) GENPYBIND(visible);

extern template struct GENPYBIND(visible) Template<Abc::N>;
} // namespace nested
