// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool -dump-graph=visibility %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

#include <type_traits>

struct Example {};

// CHECK:      Declaration context graph (unpruned) with visibility of all nodes:
// CHECK-NOT: Namespace 'std'
// CHECK-NOT: Namespace '__gnu_cxx'
// CHECK-NEXT: `-CXXRecord 'Example': hidden
