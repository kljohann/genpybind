// RUN: genpybind-tool -dump-graph=pruned %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

namespace prune_me {
struct X {};
} // namespace prune_me

using MovedHere GENPYBIND(expose_here) = prune_me::X;

// CHECK:      Declaration context graph after pruning:
// CHECK-NEXT: `-CXXRecord 'prune_me::X' as 'MovedHere': visible
