#include "args-and-kwargs.h"

#include <pybind11/pybind11.h>

int number_of_arguments(alias::args args, alias::kwargs kwargs) {
  return static_cast<int>(pybind11::len(args) + pybind11::len(kwargs));
}

int mixed(int offset, alias::args args, alias::kwargs kwargs) {
  return offset + number_of_arguments(args, kwargs);
}
