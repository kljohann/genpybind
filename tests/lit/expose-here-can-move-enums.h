// RUN: genpybind-tool -dump-graph=pruned %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

namespace nested {
enum class ScopedEnum {
};
enum UnscopedEnum {
};
}  // namespace nested

struct GENPYBIND(visible) Outer {
  using Unscoped GENPYBIND(expose_here) = nested::UnscopedEnum;
  using Scoped GENPYBIND(expose_here) = nested::ScopedEnum;
};

// CHECK:      Declaration context graph after pruning:
// CHECK-NEXT: `-CXXRecord 'Outer': visible
// CHECK-NEXT:   |-Enum 'nested::UnscopedEnum' as 'Unscoped': visible
// CHECK-NEXT:   `-Enum 'nested::ScopedEnum' as 'Scoped': visible
