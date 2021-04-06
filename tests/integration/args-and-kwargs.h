#pragma once

#include "genpybind.h"

namespace pybind11 {
class args;
class kwargs;
} // namespace pybind11

// args and kwargs should be recognized despite the use of an alias.
namespace alias = pybind11;

GENPYBIND(visible)
int number_of_arguments(alias::args args, alias::kwargs kwargs);

GENPYBIND(visible)
int mixed(int offset, alias::args args, alias::kwargs kwargs);
