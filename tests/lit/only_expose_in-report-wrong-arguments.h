// RUN: genpybind-tool --xfail %s -- 2>&1 \
// RUN: | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

// CHECK: arguments.h:[[# @LINE + 1]]:11: error: Wrong number of arguments for 'only_expose_in' annotation
namespace something GENPYBIND(only_expose_in()) {}

// CHECK: 1 error generated.
