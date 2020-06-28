// RUN: genpybind-tool -dump-graph=visibility %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

struct ShouldBeVisible {
  struct ShouldAlsoBeVisible {};
};

struct GENPYBIND(visible) ExplicitlyVisible {
  struct ImplicitlyVisible {
    using ShouldBeVisible GENPYBIND(expose_here) = ShouldBeVisible;
  };
};

// CHECK:      Declaration context graph after visibility propagation:
// CHECK-NEXT: `-CXXRecord 'ExplicitlyVisible': visible
// CHECK-NEXT:   `-CXXRecord 'ExplicitlyVisible::ImplicitlyVisible': visible
// CHECK-NEXT:     `-CXXRecord 'ShouldBeVisible': visible, expose_as("ShouldBeVisible")
// CHECK-NEXT:       `-CXXRecord 'ShouldBeVisible::ShouldAlsoBeVisible': visible
