// RUN: genpybind-tool -dump-graph=visibility %s -- -std=c++17 -xc++ \
// RUN: -D__GENPYBIND__ 2>&1 | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

#include <type_traits>

struct Example {};

// CHECK:      Declaration context graph (unpruned) with visibility of all nodes:
// CHECK-NOT: Namespace 'std'
// CHECK-NOT: Namespace '__gnu_cxx'
// CHECK-NEXT: `-CXXRecord 'Example': hidden
