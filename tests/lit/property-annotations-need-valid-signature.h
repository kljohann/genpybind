// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

struct Example {
  GENPYBIND(getter_for("rating"))
  int getRating() const;

  GENPYBIND(getter_for("size"))
  int getSize(bool optional = true) const;

  GENPYBIND(getter_for("invalid"))
  // CHECK: signature.h:[[# @LINE + 1]]:7: error: Signature of method is incompatible with 'getter_for' annotation
  int invalidSignature(bool mandatory) const;

  GENPYBIND(getter_for("whatever"))
  // CHECK: signature.h:[[# @LINE + 1]]:8: error: Signature of method is incompatible with 'getter_for' annotation
  void invalidReturnType() const;

  GENPYBIND(setter_for("rating"))
  void setRating(int value);

  GENPYBIND(setter_for("size"))
  void setSize(int value, bool optional = true);

  GENPYBIND(setter_for("height"))
  void setSize(int value = 0);

  GENPYBIND(setter_for("invalid"))
  // CHECK: signature.h:[[# @LINE + 1]]:8: error: Signature of method is incompatible with 'setter_for' annotation
  void invalidSignatureMissingArguments();

  GENPYBIND(setter_for("invalid"))
  // CHECK: signature.h:[[# @LINE + 1]]:8: error: Signature of method is incompatible with 'setter_for' annotation
  void invalidSignature(bool mandatory, int also_mandatory);
};

// CHECK: 4 errors generated.
