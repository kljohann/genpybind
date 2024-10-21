// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool -dump-graph=pruned %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

template <int Value> struct NonTypeParameter {};

typedef NonTypeParameter<123> Test123 GENPYBIND(expose_here);
typedef NonTypeParameter<42> Test42 GENPYBIND(expose_here);

struct X {};
struct Y {};

template <typename T> struct TypeParameter {};

typedef TypeParameter<X> TestX GENPYBIND(expose_here);
typedef TypeParameter<Y> TestY GENPYBIND(expose_here);

// CHECK:      Declaration context graph after pruning:
// CHECK-NEXT: |-ClassTemplateSpecialization 'NonTypeParameter<123>' as 'Test123': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'NonTypeParameter<42>' as 'Test42': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'TypeParameter<X>' as 'TestX': visible
// CHECK-NEXT: `-ClassTemplateSpecialization 'TypeParameter<Y>' as 'TestY': visible
