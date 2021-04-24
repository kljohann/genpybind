// RUN: genpybind-tool %s -- 2>&1 \
// RUN: | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

struct GENPYBIND(visible) Target {};

struct GENPYBIND(visible) Context {
  // CHECK: aliases.h:[[# @LINE + 1]]:24: warning: Ignoring qualifiers on alias
  typedef const Target td_with_qualifier GENPYBIND(visible);
  // CHECK-NEXT: typedef const Target td_with_qualifier GENPYBIND(visible);
  // CHECK-NEXT: ~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~~~~

  // CHECK: aliases.h:[[# @LINE + 1]]:9: warning: Ignoring qualifiers on alias
  using alias_with_qualifier GENPYBIND(visible) = const Target;
};
// CHECK: 2 warnings generated.
