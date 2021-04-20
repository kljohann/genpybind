// RUN: genpybind-tool --xfail %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 \
// RUN: | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

// CHECK: arguments.h:[[# @LINE + 1]]:11: error: Wrong number of arguments for 'only_expose_in' annotation
namespace something GENPYBIND(only_expose_in()) {}

// CHECK: 1 error generated.
