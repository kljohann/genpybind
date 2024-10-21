// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

struct Target {};

// CHECK: conflicting-annotations.h:[[# @LINE + 1]]:7: error: 'encourage' and 'expose_here' cannot be used at the same time
using Alias GENPYBIND(encourage, expose_here) = Target;

// CHECK: 1 error generated.
