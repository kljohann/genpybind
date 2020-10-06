// RUN: genpybind-tool --xfail %s -- -std=c++17 -xc++ -D__GENPYBIND__ 2>&1 \
// RUN: | FileCheck %s --strict-whitespace
#pragma once

#include "genpybind.h"

// TODO: Diagnostics should be emitted in the right order (-> add line number checks then).

struct GENPYBIND(visible) Example {
  int getter_defined_twice() GENPYBIND(getter_for("getter_twice"));
  // CHECK-DAG: :7: error: getter already defined for 'getter_twice'
  int getter_defined_twice_2() GENPYBIND(getter_for("getter_twice"));

  int setter_defined_twice() GENPYBIND(getter_for("setter_twice"));
  void setter_defined_twice(int value) GENPYBIND(setter_for("setter_twice"));
  // CHECK-DAG: :8: error: setter already defined for 'setter_twice'
  void setter_defined_twice_2(int value) GENPYBIND(setter_for("setter_twice"));

  int both_defined_twice() GENPYBIND(getter_for("both_twice"));
  // CHECK-DAG: :7: error: getter already defined for 'both_twice'
  int both_defined_twice_2() GENPYBIND(getter_for("both_twice"));
  void both_defined_twice(int value) GENPYBIND(setter_for("both_twice"));
  // CHECK-DAG: :8: error: setter already defined for 'both_twice'
  void both_defined_twice_2(int value) GENPYBIND(setter_for("both_twice"));

  // CHECK-DAG: :8: error: No getter for the 'prop' property
  void setter_without_getter(int value) GENPYBIND(setter_for("prop"));
};

// CHECK: 5 errors generated.
