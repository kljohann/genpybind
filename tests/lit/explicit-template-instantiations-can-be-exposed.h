// RUN: genpybind-tool -dump-graph=pruned %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

template <typename T>
struct ExposeSomeInstantiations {};

extern template struct GENPYBIND(expose_as(IntSomething)) ExposeSomeInstantiations<int>;

template struct GENPYBIND(expose_as(FloatSomething)) ExposeSomeInstantiations<float>;

template struct GENPYBIND(visible) ExposeSomeInstantiations<bool>;

template struct ExposeSomeInstantiations<double>; // not exposed

template <typename T>
struct GENPYBIND(visible) ExposeAll {};

extern template struct ExposeAll<int>;
template struct ExposeAll<float>;
template struct GENPYBIND(expose_as(BoolSomething)) ExposeAll<bool>;

// TODO: Should it be possible to selectively hide some instantiations?
template struct GENPYBIND(hidden) ExposeAll<double>;

// CHECK:      Declaration context graph after pruning:
// CHECK-NEXT: |-ClassTemplateSpecialization 'ExposeSomeInstantiations<int>' as 'IntSomething': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'ExposeSomeInstantiations<float>' as 'FloatSomething': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'ExposeSomeInstantiations<bool>': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'ExposeAll<int>': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'ExposeAll<float>': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'ExposeAll<bool>' as 'BoolSomething': visible
// NOTE/FIXME: This instantiation is visible, despite the 'hidden' annotation.
// CHECK-NEXT: `-ClassTemplateSpecialization 'ExposeAll<double>': visible
