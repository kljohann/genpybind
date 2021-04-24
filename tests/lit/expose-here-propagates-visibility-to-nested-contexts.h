// RUN: genpybind-tool -dump-graph=visibility %s -- 2>&1 \
// RUN: | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

struct ShouldBeVisible {
  struct ShouldAlsoBeVisible {};
};

struct GENPYBIND(visible) ExplicitlyVisible {
  struct ImplicitlyVisible {
    using ExposedHere GENPYBIND(expose_here) = ShouldBeVisible;
  };
};

// CHECK:      Declaration context graph (unpruned) with visibility of all nodes:
// CHECK-NEXT: `-CXXRecord 'ExplicitlyVisible': visible
// CHECK-NEXT:   `-CXXRecord 'ExplicitlyVisible::ImplicitlyVisible': visible
// CHECK-NEXT:     `-CXXRecord 'ShouldBeVisible' as 'ExposedHere': visible
// CHECK-NEXT:       `-CXXRecord 'ShouldBeVisible::ShouldAlsoBeVisible': visible
