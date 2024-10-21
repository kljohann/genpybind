// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --dump-graph=visibility %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace --match-full-lines

#pragma once

#include <genpybind/genpybind.h>

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
