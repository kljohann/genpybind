// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool -dump-graph=pruned %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

template <typename T> struct ExposeSomeInstantiations {};

extern template struct GENPYBIND(expose_as(IntSomething))
    ExposeSomeInstantiations<int>;

template struct GENPYBIND(expose_as(FloatSomething))
    ExposeSomeInstantiations<float>;

template struct GENPYBIND(visible) ExposeSomeInstantiations<bool>;

template struct ExposeSomeInstantiations<double>; // not exposed

template <typename T> struct GENPYBIND(visible) ExposeAll {};

extern template struct ExposeAll<int>;
template struct ExposeAll<float>;
template struct GENPYBIND(expose_as(BoolSomething)) ExposeAll<bool>;

// TODO: Should it be possible to selectively hide some instantiations?
template struct GENPYBIND(hidden) ExposeAll<double>;

// CHECK:      Declaration context graph after pruning:
// CHECK-NEXT: |-ClassTemplateSpecialization 'ExposeSomeInstantiations<int>' as 'IntSomething': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'ExposeSomeInstantiations<float>' as 'FloatSomething': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'ExposeSomeInstantiations<bool>' as 'ExposeSomeInstantiations_bool_': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'ExposeAll<int>' as 'ExposeAll_int_': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'ExposeAll<float>' as 'ExposeAll_float_': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'ExposeAll<bool>' as 'BoolSomething': visible
// NOTE/FIXME: This instantiation is visible, despite the 'hidden' annotation.
// CHECK-NEXT: `-ClassTemplateSpecialization 'ExposeAll<double>' as 'ExposeAll_double_': visible
