// RUN: genpybind-tool %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

struct GENPYBIND(visible) Base {};

// CHECK: base-class.h:[[# @LINE + 3]]:49: warning: Unknown base type in 'hide_base' annotation
// CHECK-NEXT: struct GENPYBIND(visible, hide_base("Missing")) Derived : Base {};
// CHECK-NEXT: ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~~~~
struct GENPYBIND(visible, hide_base("Missing")) Derived : Base {};

// CHECK: 1 warning generated.
