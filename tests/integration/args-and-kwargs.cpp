// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "args-and-kwargs.h"

#include <pybind11/pybind11.h>

#include <utility>

int number_of_arguments(const alias::args &args, const alias::kwargs &kwargs) {
  return static_cast<int>(pybind11::len(args) + pybind11::len(kwargs));
}

int mixed(int offset, const alias::args &args, const alias::kwargs &kwargs) {
  return offset + number_of_arguments(args, kwargs);
}

int trailing_arguments(int offset, const alias::args &args, int multiplier) {
  return multiplier * (offset + static_cast<int>(pybind11::len(args)));
}

int by_value(alias::args args, alias::kwargs kwargs) {
  return static_cast<int>(pybind11::len(std::move(args)) +
                          pybind11::len(std::move(kwargs)));
}
