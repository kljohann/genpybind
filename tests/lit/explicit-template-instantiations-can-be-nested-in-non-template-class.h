// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool -dump-graph=visibility -dump-graph=pruned %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

struct GENPYBIND(visible) Outer {
  template <typename T> struct Type {
    enum class Whatever { A, B, C };
  };
};

template struct GENPYBIND(visible) Outer::Type<bool>;

extern template struct GENPYBIND(visible) Outer::Type<float>;

using InstantiationViaAlias GENPYBIND(expose_here) = Outer::Type<int>;

extern template struct Outer::Type<void>;
using InstantiationOfDeclaredViaAlias
    GENPYBIND(expose_here) = Outer::Type<void>;

struct GENPYBIND(visible) Stop {};

// CHECK:      Declaration context graph (unpruned) with visibility of all nodes:
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer::Type<int>' as 'InstantiationViaAlias': visible
// CHECK-NEXT: | `-Enum 'Outer::Type<int>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer::Type<void>' as 'InstantiationOfDeclaredViaAlias': visible
// CHECK-NEXT: | `-Enum 'Outer::Type<void>::Whatever': visible
// CHECK-NEXT: |-CXXRecord 'Outer': visible
// CHECK-NEXT: | |-ClassTemplateSpecialization 'Outer::Type<bool>' as 'Type_bool_': visible
// CHECK-NEXT: | | `-Enum 'Outer::Type<bool>::Whatever': visible
// CHECK-NEXT: | `-ClassTemplateSpecialization 'Outer::Type<float>' as 'Type_float_': visible
// CHECK-NEXT: |   `-Enum 'Outer::Type<float>::Whatever': visible
// CHECK-NEXT: `-CXXRecord 'Stop': visible

// CHECK:      Declaration context graph after pruning:
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer::Type<int>' as 'InstantiationViaAlias': visible
// CHECK-NEXT: | `-Enum 'Outer::Type<int>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer::Type<void>' as 'InstantiationOfDeclaredViaAlias': visible
// CHECK-NEXT: | `-Enum 'Outer::Type<void>::Whatever': visible
// CHECK-NEXT: |-CXXRecord 'Outer': visible
// CHECK-NEXT: | |-ClassTemplateSpecialization 'Outer::Type<bool>' as 'Type_bool_': visible
// CHECK-NEXT: | | `-Enum 'Outer::Type<bool>::Whatever': visible
// CHECK-NEXT: | `-ClassTemplateSpecialization 'Outer::Type<float>' as 'Type_float_': visible
// CHECK-NEXT: |   `-Enum 'Outer::Type<float>::Whatever': visible
// CHECK-NEXT: `-CXXRecord 'Stop': visible
