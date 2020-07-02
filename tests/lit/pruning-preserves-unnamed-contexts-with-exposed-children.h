// RUN: genpybind-tool -dump-graph=pruned %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

#define GENPYBIND_VISIBLE GENPYBIND(visible)
#define GENPYBIND_HIDDEN GENPYBIND(hidden)

namespace prune_me {
namespace visible GENPYBIND_VISIBLE {
namespace hidden GENPYBIND_HIDDEN {

struct Unused {
  struct GENPYBIND(visible) Unreachable {};
};

} // namespace GENPYBIND_HIDDEN
} // namespace GENPYBIND_VISIBLE
} // namespace prune_me

namespace not_pruned {
namespace visible GENPYBIND_VISIBLE {
namespace hidden GENPYBIND_HIDDEN {

struct GENPYBIND(visible) Exposed {};

} // namespace GENPYBIND_HIDDEN
} // namespace GENPYBIND_VISIBLE
} // namespace not_pruned

namespace also_not_pruned {
namespace visible GENPYBIND_VISIBLE {

extern "C" {
GENPYBIND(visible(default)) void visible_default_function();
}

namespace hidden_but_should_not_be_pruned GENPYBIND_HIDDEN {

extern const int some_constant GENPYBIND(visible);

extern "C" {
GENPYBIND(visible) void lexically_nested_visible_function();
}

extern "C" {
extern "C" {
using Alias GENPYBIND(visible) = not_pruned::visible::hidden::Exposed;
}
}

} // namespace GENPYBIND_HIDDEN
} // namespace GENPYBIND_VISIBLE
} // namespace also_not_pruned

extern "C" {
GENPYBIND(visible) void visible_function();
}

// CHECK:      Declaration context graph after pruning:
// CHECK-NOT: prune_me
// CHECK-NOT: Unused
// CHECK-NOT: Unreachable
// CHECK:      |-Namespace 'not_pruned':
// CHECK-NEXT: | `-Namespace 'not_pruned::visible': visible
// CHECK-NEXT: |   `-Namespace 'not_pruned::visible::hidden': hidden
// CHECK-NEXT: |     `-CXXRecord 'not_pruned::visible::hidden::Exposed': visible
// CHECK-NEXT: |-Namespace 'also_not_pruned':
// CHECK-NEXT: | `-Namespace 'also_not_pruned::visible': visible
// CHECK-NEXT: |   |-LinkageSpec:
// CHECK-NEXT: |   `-Namespace 'also_not_pruned::visible::hidden_but_should_not_be_pruned': hidden
// CHECK-NEXT: |     |-LinkageSpec:
// CHECK-NEXT: |     `-LinkageSpec:
// CHECK-NEXT: |       `-LinkageSpec:
// CHECK-NEXT: `-LinkageSpec:
