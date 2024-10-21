// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --dump-graph=visibility %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

template <typename Derived, typename T> struct CRTP : public T {};

// CHECK-NOT: Unknown base type in 'inline_base' annotation
// CHECK-NOT: Invalid assumption in genpybind
template <typename Derived, typename T>
struct GENPYBIND(inline_base(CRTP), hidden) SecondLevel
    : public CRTP<Derived, T> {};

// CHECK: Declaration context graph (unpruned)
// CHECK-NEXT: <no children>

// CHECK-NOT: warning generated
// CHECK-NOT: warnings generated
