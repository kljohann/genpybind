// RUN: genpybind-tool %s -- 2>&1 \
// RUN: | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

struct GENPYBIND(visible) Base {};

// CHECK: base-class.h:[[# @LINE + 3]]:49: warning: Unknown base type in 'hide_base' annotation
// CHECK-NEXT: struct GENPYBIND(visible, hide_base("Missing")) Derived : Base {};
// CHECK-NEXT: ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~~~~
struct GENPYBIND(visible, hide_base("Missing")) Derived : Base {};

// CHECK: 1 warning generated.
