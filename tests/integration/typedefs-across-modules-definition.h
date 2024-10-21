// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

namespace example
GENPYBIND(only_expose_in(typedefs_across_modules_definition)) {
namespace nested {

struct GENPYBIND(visible) Definition {};
using Alias GENPYBIND(visible) = Definition;
struct EncouragedFromOtherModule {};
struct WorkingEncouragedFromOtherModule {};
struct ExposedInOtherModule {};

} // namespace nested
} // namespace example

namespace other GENPYBIND(only_expose_in(typedefs_across_modules)) {
using WorkingAliasToEncouraged
    GENPYBIND(encourage) = example::nested::WorkingEncouragedFromOtherModule;
} // namespace other
