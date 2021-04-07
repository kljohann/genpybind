// RUN: genpybind-tool --dump-graph=pruned %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

namespace other {
inline namespace v1 {
struct something {};
} // namespace v1
} // namespace other

// CHECK: Declaration context graph after pruning:
// CHECK-NEXT: <no children>
