// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --dump-graph=pruned %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

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

namespace hidden_but_should_not_be_pruned_1 GENPYBIND_HIDDEN {

extern const int some_constant GENPYBIND(visible);

} // namespace GENPYBIND_HIDDEN

namespace hidden_but_should_not_be_pruned_2 GENPYBIND_HIDDEN {

extern "C" {
GENPYBIND(visible) void nested_visible_function();
}

} // namespace GENPYBIND_HIDDEN

namespace hidden_but_should_not_be_pruned_3 GENPYBIND_HIDDEN {

extern "C" {
extern "C" {
using Alias GENPYBIND(visible) = not_pruned::visible::hidden::Exposed;
}
}

} // namespace GENPYBIND_HIDDEN
} // namespace GENPYBIND_VISIBLE
} // namespace also_not_pruned

// CHECK:      Declaration context graph after pruning:
// CHECK-NOT: prune_me
// CHECK-NOT: Unused
// CHECK-NOT: Unreachable
// CHECK:      |-Namespace 'not_pruned': hidden
// CHECK-NEXT: | `-Namespace 'not_pruned::visible': visible
// CHECK-NEXT: |   `-Namespace 'not_pruned::visible::hidden': hidden
// CHECK-NEXT: |     `-CXXRecord 'not_pruned::visible::hidden::Exposed': visible
// CHECK-NEXT: `-Namespace 'also_not_pruned': hidden
// CHECK-NEXT:   `-Namespace 'also_not_pruned::visible': visible
// CHECK-NEXT:     |-Namespace 'also_not_pruned::visible::hidden_but_should_not_be_pruned_1': hidden
// CHECK-NEXT:     |-Namespace 'also_not_pruned::visible::hidden_but_should_not_be_pruned_2': hidden
// CHECK-NEXT:     `-Namespace 'also_not_pruned::visible::hidden_but_should_not_be_pruned_3': hidden
