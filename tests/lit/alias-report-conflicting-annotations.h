// RUN: genpybind-tool --xfail %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s
#pragma once

#include "genpybind.h"

struct Target {};

// CHECK: conflicting-annotations.h:[[# @LINE + 1]]:7: error: 'encourage' and 'expose_here' cannot be used at the same time
using Alias GENPYBIND(encourage, expose_here) = Target;

// CHECK: 1 error generated.
