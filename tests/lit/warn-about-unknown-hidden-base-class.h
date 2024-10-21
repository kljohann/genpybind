// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

struct GENPYBIND(visible) Base {};

// CHECK: base-class.h:[[# @LINE + 3]]:49: warning: Unknown base type in 'hide_base' annotation
// CHECK-NEXT: struct GENPYBIND(visible, hide_base("Missing")) Derived : Base {};
// CHECK-NEXT: ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~~~~
struct GENPYBIND(visible, hide_base("Missing")) Derived : Base {};

// CHECK: 1 warning generated.
