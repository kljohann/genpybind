// RUN: genpybind-tool --dump-graph=visibility %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 | FileCheck %s --strict-whitespace --match-full-lines
#pragma once

#include "genpybind.h"

namespace foo {
namespace bar GENPYBIND(module) {
  class X;
} // namespace )

// The scope / point of exposure is determined by the semantic context of X's
// definition, not by its lexical context.
class GENPYBIND(visible) bar::X {};

} // namespace foo

//      CHECK:Declaration context graph (unpruned) with visibility of all nodes:
// CHECK-NEXT:`-Namespace 'foo': hidden
// CHECK-NEXT:  `-Namespace 'foo::bar': hidden
// CHECK-NEXT:    `-CXXRecord 'foo::bar::X': visible