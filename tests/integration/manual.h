#pragma once

#ifdef __GENPYBIND__
#pragma genpybind include <pybind11/pybind11.h>
#pragma genpybind include <pybind11/eval.h>
#endif // __GENPYBIND__
#include "genpybind.h"

namespace nested {

struct GENPYBIND(visible) Example {
  bool method() GENPYBIND(hidden);
  bool values[2] GENPYBIND(hidden) = {false, false};

  GENPYBIND_MANUAL({ parent.def("manual_method", &::nested::Example::method); })

  GENPYBIND_MANUAL({
    using Example = GENPYBIND_PARENT_TYPE;
    parent.def("__getitem__",
               [](Example &self, bool key) { return self.values[key]; });
    parent.def("__setitem__", [](Example &self, bool key, bool value) {
      self.values[key] = value;
    });
  })
};

struct Hidden {};

GENPYBIND_MANUAL({
  auto context = ::pybind11::class_<::nested::Hidden>(parent, "Hidden");
  context.def(::pybind11::init<>());
})

} // namespace nested
