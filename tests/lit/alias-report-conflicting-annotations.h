// RUN: genpybind-tool --xfail %s -- 2>&1 \
// RUN: | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

struct Target {};

// CHECK: conflicting-annotations.h:[[# @LINE + 1]]:7: error: 'encourage' and 'expose_here' cannot be used at the same time
using Alias GENPYBIND(encourage, expose_here) = Target;

// CHECK: 1 error generated.
