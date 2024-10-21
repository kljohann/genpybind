// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool -dump-graph=pruned %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

namespace prune_me {
struct X {};
} // namespace prune_me

using MovedHere GENPYBIND(expose_here) = prune_me::X;

// CHECK:      Declaration context graph after pruning:
// CHECK-NEXT: `-CXXRecord 'prune_me::X' as 'MovedHere': visible
