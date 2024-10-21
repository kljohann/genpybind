// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool -dump-graph=visibility -dump-graph=pruned %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

template <typename T> struct GENPYBIND(visible) Outer {
  template <typename V> struct Inner {
    enum class Whatever { A, B, C };
  };
};

template struct GENPYBIND(visible) Outer<void>::Inner<bool>;

template struct Outer<int>;
template struct GENPYBIND(visible) Outer<int>::Inner<bool>;

extern template struct GENPYBIND(visible) Outer<void>::Inner<float>;

using InstantiationViaAlias GENPYBIND(expose_here) = Outer<bool>::Inner<int>;

extern template struct Outer<double>::Inner<void>;
using InstantiationOfDeclaredViaAlias
    GENPYBIND(expose_here) = Outer<double>::Inner<void>;

struct GENPYBIND(visible) Stop {};

// CHECK:      Declaration context graph (unpruned) with visibility of all nodes:
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer<bool>::Inner<int>' as 'InstantiationViaAlias': visible
// CHECK-NEXT: | `-Enum 'Outer<bool>::Inner<int>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer<double>::Inner<void>' as 'InstantiationOfDeclaredViaAlias': visible
// CHECK-NEXT: | `-Enum 'Outer<double>::Inner<void>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer<int>' as 'Outer_int_': visible
// CHECK-NEXT: | `-ClassTemplateSpecialization 'Outer<int>::Inner<bool>' as 'Inner_bool_': visible
// CHECK-NEXT: |   `-Enum 'Outer<int>::Inner<bool>::Whatever': visible
// CHECK-NEXT: |-CXXRecord 'Stop': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer<void>' as 'Outer_void_': visible
// CHECK-NEXT: | |-ClassTemplateSpecialization 'Outer<void>::Inner<bool>' as 'Inner_bool_': visible
// CHECK-NEXT: | | `-Enum 'Outer<void>::Inner<bool>::Whatever': visible
// CHECK-NEXT: | `-ClassTemplateSpecialization 'Outer<void>::Inner<float>' as 'Inner_float_': visible
// CHECK-NEXT: |   `-Enum 'Outer<void>::Inner<float>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer<bool>' as 'Outer_bool_': visible
// CHECK-NEXT: `-ClassTemplateSpecialization 'Outer<double>' as 'Outer_double_': visible

// CHECK:      Declaration context graph after pruning:
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer<bool>::Inner<int>' as 'InstantiationViaAlias': visible
// CHECK-NEXT: | `-Enum 'Outer<bool>::Inner<int>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer<double>::Inner<void>' as 'InstantiationOfDeclaredViaAlias': visible
// CHECK-NEXT: | `-Enum 'Outer<double>::Inner<void>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer<int>' as 'Outer_int_': visible
// CHECK-NEXT: | `-ClassTemplateSpecialization 'Outer<int>::Inner<bool>' as 'Inner_bool_': visible
// CHECK-NEXT: |   `-Enum 'Outer<int>::Inner<bool>::Whatever': visible
// CHECK-NEXT: |-CXXRecord 'Stop': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer<void>' as 'Outer_void_': visible
// CHECK-NEXT: | |-ClassTemplateSpecialization 'Outer<void>::Inner<bool>' as 'Inner_bool_': visible
// CHECK-NEXT: | | `-Enum 'Outer<void>::Inner<bool>::Whatever': visible
// CHECK-NEXT: | `-ClassTemplateSpecialization 'Outer<void>::Inner<float>' as 'Inner_float_': visible
// CHECK-NEXT: |   `-Enum 'Outer<void>::Inner<float>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer<bool>' as 'Outer_bool_': visible
// CHECK-NEXT: `-ClassTemplateSpecialization 'Outer<double>' as 'Outer_double_': visible
