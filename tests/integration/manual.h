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

  GENPYBIND_MANUAL({
    ::pybind11::exec(R"(
      from os import environ
      environ["order_of_execution"] = environ.get("order_of_execution", "") + ",class"
    )");
  })
};

struct Hidden {};

GENPYBIND_MANUAL({
  auto context = ::pybind11::class_<::nested::Hidden>(parent, "Hidden");
  context.def(::pybind11::init<>());
})

GENPYBIND_MANUAL({
  ::pybind11::exec(R"(
    from os import environ
    environ["order_of_execution"] = environ.get("order_of_execution", "") + ",namespace"
  )");
})

} // namespace nested

GENPYBIND(postamble)
GENPYBIND_MANUAL({
  ::pybind11::exec(R"(
    from os import environ
    environ["order_of_execution"] = environ.get("order_of_execution", "") + ",postamble"
  )");
})

GENPYBIND_MANUAL({
  auto environ = ::pybind11::module::import("os").attr("environ");
  environ.attr("setdefault")("order_of_execution", "preamble");
})
