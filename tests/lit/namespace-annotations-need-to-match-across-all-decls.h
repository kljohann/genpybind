// RUN: genpybind-tool --xfail %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 \
// RUN: | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

namespace good GENPYBIND(visible) {}
namespace good GENPYBIND(visible) {}

// clang-format off
namespace pedantic_note_missing_space GENPYBIND(visible,module) {}
// clang-format on

// CHECK: decls.h:[[# @LINE + 2]]:11: error: Annotations need to match those of the original namespace
// CHECK: decls.h:[[# @LINE - 4]]:11: note: declared here
namespace pedantic_note_missing_space GENPYBIND(visible, module) {}

namespace only_original_is_annotated GENPYBIND(visible) {}

// CHECK: decls.h:[[# @LINE + 2]]:11: error: Annotations need to match those of the original namespace
// CHECK: decls.h:[[# @LINE - 3]]:11: note: declared here
namespace only_original_is_annotated {}

namespace only_annotated_later {}

// CHECK: decls.h:[[# @LINE + 2]]:11: error: Annotations need to match those of the original namespace
// CHECK: decls.h:[[# @LINE - 3]]:11: note: declared here
namespace only_annotated_later GENPYBIND(visible) {}

namespace only_annotated_later {}

// CHECK: 3 errors generated.
