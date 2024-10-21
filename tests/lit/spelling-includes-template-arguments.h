// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool -dump-graph=pruned %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

template <int Value> struct Encouraged {};

template <int Value> struct GENPYBIND(visible) WithInteger {
  using Alias GENPYBIND(encourage) = Encouraged<Value>;
};

template struct WithInteger<123>;
template struct WithInteger<42>;

namespace with_types {
namespace arguments {
struct X {};
struct Y {};
} // namespace arguments

template <typename T> struct Encouraged {};

template <typename T> struct GENPYBIND(visible) WithType {
  using Alias GENPYBIND(encourage) = Encouraged<T>;
};

template struct WithType<arguments::X>;

} // namespace with_types

template struct with_types::WithType<with_types::arguments::Y>;

// CHECK:      Declaration context graph after pruning:
// CHECK-NEXT: |-ClassTemplateSpecialization 'WithInteger<123>' as 'WithInteger_123_': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'WithInteger<42>' as 'WithInteger_42_': visible
// CHECK-NEXT: |-Namespace 'with_types': hidden
// CHECK-NEXT: | |-ClassTemplateSpecialization 'with_types::WithType<with_types::arguments::X>' as 'WithType_with_types_arguments_X_': visible
// CHECK-NEXT: | |-ClassTemplateSpecialization 'with_types::Encouraged<with_types::arguments::X>' as 'Encouraged_with_types_arguments_X_': visible
// CHECK-NEXT: | |-ClassTemplateSpecialization 'with_types::Encouraged<with_types::arguments::Y>' as 'Encouraged_with_types_arguments_Y_': visible
// CHECK-NEXT: | `-ClassTemplateSpecialization 'with_types::WithType<with_types::arguments::Y>' as 'WithType_with_types_arguments_Y_': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Encouraged<123>' as 'Encouraged_123_': visible
// CHECK-NEXT: `-ClassTemplateSpecialization 'Encouraged<42>' as 'Encouraged_42_': visible
