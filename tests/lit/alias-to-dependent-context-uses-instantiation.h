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

// CHECK:      Declaration context graph (unpruned) with visibility of all nodes:
// CHECK-NEXT: |-ClassTemplateSpecialization 'Something<Encouraged>' as 'Test': visible
// CHECK-NEXT: |-ClassTemplateSpecialization 'Outer<Encouraged>' as 'TestOuter': visible
// CHECK-NEXT: | |-ClassTemplateSpecialization 'Something<Outer<Encouraged>::Inner>' as 'y_type': visible
// CHECK-NEXT: | `-CXXRecord 'Outer<Encouraged>::Inner': visible
// CHECK-NEXT: `-CXXRecord 'Encouraged': visible
