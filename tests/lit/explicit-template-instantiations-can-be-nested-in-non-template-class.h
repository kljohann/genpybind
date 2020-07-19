// RUN: genpybind-tool -dump-graph=visibility -dump-graph=pruned %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

struct Outer {
  template <typename T> struct Type {
    enum class Whatever { A, B, C };
  };
};

// TODO: These should probably be nested inside `Outer` in the declaration
// context graph!?

template struct GENPYBIND(visible) Outer::Type<bool>;

extern template struct GENPYBIND(visible) Outer::Type<float>;

using InstantiationViaAlias GENPYBIND(expose_here) = Outer::Type<int>;

extern template struct Outer::Type<void>;
using InstantiationOfDeclaredViaAlias GENPYBIND(expose_here) = Outer::Type<void>;

struct GENPYBIND(visible) Stop {};

// CHECK:      Declaration context graph (unpruned) with visibility of all nodes:
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer::Type<int>' as 'InstantiationViaAlias': visible
// CHECK-NEXT: | `-Enum 'Outer::Type<int>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer::Type<void>' as 'InstantiationOfDeclaredViaAlias': visible
// CHECK-NEXT: | `-Enum 'Outer::Type<void>::Whatever': visible
// CHECK-NEXT: |-CXXRecord 'Outer': hidden
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer::Type<bool>' as 'Type_bool_': visible
// CHECK-NEXT: | `-Enum 'Outer::Type<bool>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer::Type<float>' as 'Type_float_': visible
// CHECK-NEXT: | `-Enum 'Outer::Type<float>::Whatever': visible
// CHECK-NEXT: `-CXXRecord 'Stop': visible

// CHECK:      Declaration context graph after pruning:
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer::Type<int>' as 'InstantiationViaAlias': visible
// CHECK-NEXT: | `-Enum 'Outer::Type<int>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer::Type<void>' as 'InstantiationOfDeclaredViaAlias': visible
// CHECK-NEXT: | `-Enum 'Outer::Type<void>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer::Type<bool>' as 'Type_bool_': visible
// CHECK-NEXT: | `-Enum 'Outer::Type<bool>::Whatever': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer::Type<float>' as 'Type_float_': visible
// CHECK-NEXT: | `-Enum 'Outer::Type<float>::Whatever': visible
// CHECK-NEXT: `-CXXRecord 'Stop': visible
