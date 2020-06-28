// RUN: genpybind-tool -dump-graph=visibility %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

struct Encouraged {};

template <typename X>
struct Something {
  typedef X x_type GENPYBIND(encourage);
};

typedef Something<Encouraged> Test GENPYBIND(expose_here);

template <typename X>
struct Outer {
  struct Inner {};
  typedef Something<Inner> y_type GENPYBIND(expose_here);
};

typedef Outer<Encouraged> TestOuter GENPYBIND(expose_here);

// CHECK:      Declaration context graph after visibility propagation:
// CHECK-NEXT: |-ClassTemplateSpecialization 'Something<Encouraged>': visible, expose_as("Test")
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer<Encouraged>': visible, expose_as("TestOuter")
// CHECK-NEXT: | |-ClassTemplateSpecialization 'Something<Outer<Encouraged>::Inner>': visible, expose_as("y_type")
// CHECK-NEXT: | `-CXXRecord 'Outer<Encouraged>::Inner': visible
// CHECK-NEXT: `-CXXRecord 'Encouraged': visible
