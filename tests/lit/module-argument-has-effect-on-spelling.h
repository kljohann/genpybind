// RUN: genpybind-tool -dump-graph=visibility %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

namespace example GENPYBIND(module) {} // namespace )

namespace example GENPYBIND(expose_as(expose_as),
                            module(later_module_arg_wins)) {} // namespace )

namespace example GENPYBIND(module(module_arg),
                            expose_as(later_expose_as_wins)) {} // namespace )

// CHECK:     Declaration context graph (unpruned) with visibility of all nodes:
// CHECK:      |-Namespace 'example': hidden
// CHECK-NEXT: |-Namespace 'example' as 'later_module_arg_wins': hidden
// CHECK-NEXT: `-Namespace 'example' as 'later_expose_as_wins': hidden
