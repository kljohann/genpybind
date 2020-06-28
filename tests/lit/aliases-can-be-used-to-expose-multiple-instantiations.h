// RUN: genpybind-tool -dump-graph=pruned %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

template <int Value> struct NonTypeParameter {};

typedef NonTypeParameter<123> Test123 GENPYBIND(expose_here);
typedef NonTypeParameter<42> Test42 GENPYBIND(expose_here);

struct X {};
struct Y {};

template <typename T> struct TypeParameter {};

typedef TypeParameter<X> TestX GENPYBIND(expose_here);
typedef TypeParameter<Y> TestY GENPYBIND(expose_here);

// CHECK:      Declaration context graph after pruning:
// CHECK-NEXT: |-ClassTemplateSpecialization 'NonTypeParameter<123>': visible, expose_as("Test123")
// CHECK-NEXT: |-ClassTemplateSpecialization 'NonTypeParameter<42>': visible, expose_as("Test42")
// CHECK-NEXT: |-ClassTemplateSpecialization 'TypeParameter<X>': visible, expose_as("TestX")
// CHECK-NEXT: `-ClassTemplateSpecialization 'TypeParameter<Y>': visible, expose_as("TestY")
