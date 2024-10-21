// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <pybind11/pybind11.h>

#include <sstream>
#include <string>
#include <typeinfo>

namespace genpybind {

/// A non-throwing variant of `pybind11::type::of<T>()`.
///
/// Emits a warning and returns `None` if `T` has not previously
/// been registered.
template <typename T> auto getObjectForType() -> ::pybind11::object {
  const ::std::type_info &type_info = typeid(T);
  bool throw_if_missing = false;
  auto handle =
      ::pybind11::detail::get_type_handle(type_info, throw_if_missing);
  if (!handle) {
    std::string name = type_info.name();
    ::pybind11::detail::clean_type_id(name);
    PyErr_WarnFormat(PyExc_Warning, /*stack_level=*/7,
                     "Reference to unknown type '%s'", name.c_str());
    return ::pybind11::none();
  }
  return ::pybind11::reinterpret_borrow<::pybind11::object>(handle);
}

template <typename T> std::string string_from_lshift(const T &obj) {
  std::ostringstream os;
  os << obj;
  return os.str();
}

} // namespace genpybind
