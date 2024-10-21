// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --dump-graph=pruned %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

namespace other {
inline namespace v1 {
struct something {};
} // namespace v1
} // namespace other

// CHECK: Declaration context graph after pruning:
// CHECK-NEXT: <no children>
