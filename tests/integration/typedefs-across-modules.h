// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

// clang-format off
#ifdef __GENPYBIND__
#pragma genpybind include<pybind11/pybind11.h>
#endif // __GENPYBIND__
// clang-format on

#include "typedefs-across-modules-definition.h"
#include <genpybind/genpybind.h>

GENPYBIND_MANUAL({
  ::pybind11::module::import("typedefs_across_modules_definition");
})

using AliasOfDefinition GENPYBIND(visible) = example::nested::Definition;
using AliasOfAlias GENPYBIND(visible) = example::nested::Alias;
using AliasToEncouraged
    GENPYBIND(encourage) = example::nested::EncouragedFromOtherModule;
using ExposedHere
    GENPYBIND(expose_here) = example::nested::ExposedInOtherModule;
