// RUN: genpybind-tool --verbose %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 \
// RUN: -DEXAMPLE_MARKER \
// RUN: | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

// CHECK:      Analyzing file {{.*}}verbose-prints-compilation-command.h {{.*}} using command
// CHECK-NEXT: -DEXAMPLE_MARKER

// CHECK:      Adjusting command for file {{.*}}verbose-prints-compilation-command.h to
// CHECK-NEXT: -DEXAMPLE_MARKER{{.*}}-fsyntax-only
