// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

namespace pybind11 {
class args;
class kwargs;
} // namespace pybind11

// args and kwargs should be recognized despite the use of an alias.
namespace alias = pybind11;

GENPYBIND(visible)
int number_of_arguments(const alias::args &args, const alias::kwargs &kwargs);

GENPYBIND(visible)
int mixed(int offset, const alias::args &args, const alias::kwargs &kwargs);

GENPYBIND(visible)
int trailing_arguments(int offset, const alias::args &args, int multiplier);

GENPYBIND(visible)
int by_value(alias::args args, alias::kwargs kwargs);
