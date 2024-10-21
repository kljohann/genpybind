// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool -dump-graph=visibility %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

namespace example_1 GENPYBIND(module) {}

namespace example_2 GENPYBIND(expose_as(expose_as),
                              module(later_module_arg_wins)) {}

namespace example_3 GENPYBIND(module(module_arg),
                              expose_as(later_expose_as_wins)) {}

// CHECK:     Declaration context graph (unpruned) with visibility of all nodes:
// CHECK:      |-Namespace 'example_1': hidden
// CHECK-NEXT: |-Namespace 'example_2' as 'later_module_arg_wins': hidden
// CHECK-NEXT: `-Namespace 'example_3' as 'later_expose_as_wins': hidden
