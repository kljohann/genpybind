// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool -dump-graph=pruned %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

namespace nested {
enum class ScopedEnum { X };
enum UnscopedEnum { Y };
} // namespace nested

struct GENPYBIND(visible) Outer {
  using Unscoped GENPYBIND(expose_here) = nested::UnscopedEnum;
  using Scoped GENPYBIND(expose_here) = nested::ScopedEnum;
};

// CHECK:      Declaration context graph after pruning:
// CHECK-NEXT: `-CXXRecord 'Outer': visible
// CHECK-NEXT:   |-Enum 'nested::UnscopedEnum' as 'Unscoped': visible
// CHECK-NEXT:   `-Enum 'nested::ScopedEnum' as 'Scoped': visible
