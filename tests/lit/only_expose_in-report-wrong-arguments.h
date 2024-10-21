// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT
//
// RUN: genpybind-tool --xfail %s -- %INCLUDES% 2>&1 \
// RUN: | FileCheck %s --strict-whitespace

#pragma once

#include <genpybind/genpybind.h>

// CHECK: arguments.h:[[# @LINE + 1]]:11: error: Wrong number of arguments for 'only_expose_in' annotation
namespace something GENPYBIND(only_expose_in()) {}

// CHECK: 1 error generated.
