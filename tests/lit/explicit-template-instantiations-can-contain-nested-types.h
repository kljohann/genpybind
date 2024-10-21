// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool -dump-graph=visibility -dump-graph=pruned %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

template <typename T> struct Type {
  struct Impl;

  struct Something;

  struct Something {
    int x;
  };

  enum class Whatever { A, B, C };
};

template struct GENPYBIND(visible) Type<bool>;

extern template struct GENPYBIND(visible) Type<float>;

using InstantiationViaAlias GENPYBIND(expose_here) = Type<int>;

extern template struct Type<void>;
using InstantiationOfDeclaredViaAlias GENPYBIND(expose_here) = Type<void>;

struct GENPYBIND(visible) Stop {};

// CHECK:      Declaration context graph (unpruned) with visibility of all nodes:
// CHECK-NEXT: |-ClassTemplateSpecialization 'Type<int>' as 'InstantiationViaAlias': visible
// CHECK-NEXT: | |-CXXRecord 'Type<int>::Something': visible
// CHECK-NEXT: | `-Enum 'Type<int>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Type<void>' as 'InstantiationOfDeclaredViaAlias': visible
// CHECK-NEXT: | |-CXXRecord 'Type<void>::Something': visible
// CHECK-NEXT: | `-Enum 'Type<void>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Type<bool>' as 'Type_bool_': visible
// CHECK-NEXT: | |-CXXRecord 'Type<bool>::Something': visible
// CHECK-NEXT: | `-Enum 'Type<bool>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Type<float>' as 'Type_float_': visible
// CHECK-NEXT: | |-CXXRecord 'Type<float>::Something': visible
// CHECK-NEXT: | `-Enum 'Type<float>::Whatever': visible
// CHECK-NEXT: `-CXXRecord 'Stop': visible

// CHECK:      Declaration context graph after pruning:
// CHECK-NEXT: |-ClassTemplateSpecialization 'Type<int>' as 'InstantiationViaAlias': visible
// CHECK-NEXT: | |-CXXRecord 'Type<int>::Something': visible
// CHECK-NEXT: | `-Enum 'Type<int>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Type<void>' as 'InstantiationOfDeclaredViaAlias': visible
// CHECK-NEXT: | |-CXXRecord 'Type<void>::Something': visible
// CHECK-NEXT: | `-Enum 'Type<void>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Type<bool>' as 'Type_bool_': visible
// CHECK-NEXT: | |-CXXRecord 'Type<bool>::Something': visible
// CHECK-NEXT: | `-Enum 'Type<bool>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Type<float>' as 'Type_float_': visible
// CHECK-NEXT: | |-CXXRecord 'Type<float>::Something': visible
// CHECK-NEXT: | `-Enum 'Type<float>::Whatever': visible
// CHECK-NEXT: `-CXXRecord 'Stop': visible
