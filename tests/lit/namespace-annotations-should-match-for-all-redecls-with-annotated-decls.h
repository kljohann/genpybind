// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

#define UNEXPOSED                                                              \
  void GENPYBIND__PRIVATE_UNIQUE__(unexposed_function)() {}
#define EXPOSED                                                                \
  void GENPYBIND__PRIVATE_UNIQUE__(exposed_function)() GENPYBIND(visible) {}

// -----------------------------------------------------------------------------

namespace okay GENPYBIND(visible) {
EXPOSED
}
namespace okay GENPYBIND(visible) {
EXPOSED
}

// -----------------------------------------------------------------------------

// not necessary to annotate namespace w/o exposed decls, ...
namespace something::common::detail {
UNEXPOSED
}

namespace something {
// so it should not complain here:
namespace common GENPYBIND(module(uiae)) {
EXPOSED
}

// nor here:
namespace common {
UNEXPOSED
}
} // namespace something

// -----------------------------------------------------------------------------

namespace other {
namespace common GENPYBIND(module(nrtd)) {
EXPOSED
}
}

namespace other::common {
UNEXPOSED
}

namespace other::common::detail {
UNEXPOSED
}

// -----------------------------------------------------------------------------

// clang-format off
// NOTE: Compared to below the verbatim annotation _text_ is different.
namespace not_pedantic GENPYBIND(visible,module(sloppy )) {
  EXPOSED
}
// clang-format on

namespace not_pedantic GENPYBIND(visible, module(sloppy)) {
EXPOSED
}

// -----------------------------------------------------------------------------

// In this case, two redecls. of a namespace (which both contain visible decls
// and correspond to the same lookup context) have different annotations, so
// there needs to be an error because they cannot be treated the same.

namespace oops GENPYBIND(visible) {
EXPOSED
}

// CHECK: decls.h:[[# @LINE + 2]]:11: error: Annotations need to match those of the first declaration
// CHECK: decls.h:[[# @LINE - 5]]:11: note: declared here
namespace oops GENPYBIND(visible, module(oopsie)) {
EXPOSED
}

// -----------------------------------------------------------------------------

namespace only_original_is_annotated GENPYBIND(visible) {
EXPOSED
}

// CHECK: decls.h:[[# @LINE + 2]]:11: error: Annotations need to match those of the first declaration
// CHECK: decls.h:[[# @LINE - 5]]:11: note: declared here
namespace only_original_is_annotated {
EXPOSED
}

// -----------------------------------------------------------------------------

namespace only_annotated_later {
EXPOSED
}

// CHECK: decls.h:[[# @LINE + 2]]:11: error: Annotations need to match those of the first declaration
// CHECK: decls.h:[[# @LINE - 5]]:11: note: declared here
namespace only_annotated_later GENPYBIND(visible) {
EXPOSED
}

namespace only_annotated_later {
EXPOSED
}

// -----------------------------------------------------------------------------

namespace xyz {
namespace common GENPYBIND(module(xyz1)) {
EXPOSED
}
}

// TODO: It would be better to report these in the correct order (outer namespace first).
// For 'common': module(xyz3) instead of module(xyz1)
// CHECK: decls.h:[[# @LINE + 6]]:11: error: Annotations need to match those of the first declaration
// CHECK: decls.h:[[# @LINE - 8]]:11: note: declared here
// For 'xyz': module(xyz2) instead of no annotation
// CHECK: decls.h:[[# @LINE + 2]]:11: error: Annotations need to match those of the first declaration
// CHECK: decls.h:[[# @LINE - 12]]:11: note: declared here
namespace xyz GENPYBIND(module(xyz2)) {
namespace common GENPYBIND(module(xyz3)) {
EXPOSED
}
}

// -----------------------------------------------------------------------------

namespace abc {
namespace common GENPYBIND(module(abc1)) {
EXPOSED
}
}

// TODO: It would be better to report these in the correct order (outer namespace first).
// For 'common': module(abc2) instead of module(abc1)
// CHECK: decls.h:[[# @LINE + 3]]:11: error: Annotations need to match those of the first declaration
// CHECK: decls.h:[[# @LINE - 8]]:11: note: declared here
namespace abc {
namespace common GENPYBIND(module(abc2)) {
EXPOSED
}
}

// CHECK: 6 errors generated.
