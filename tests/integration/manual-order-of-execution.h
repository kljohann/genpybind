// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

// clang-format off
#ifdef __GENPYBIND__
#pragma genpybind include<pybind11/pybind11.h>
#pragma genpybind include<pybind11/eval.h>
#endif // __GENPYBIND__
// clang-format on
#include <genpybind/genpybind.h>

GENPYBIND_MANUAL({
  ::pybind11::exec(R"(
    import os

    def track_order(name):
        order = os.environ.get("order_of_execution", "").split(",")
        order.append(name)
        os.environ["order_of_execution"] = ",".join(order)

    track_order(":: [early]")
  )");
})

namespace reopened_namespace {

GENPYBIND_MANUAL({
  ::pybind11::exec(R"(track_order("::reopened_namespace [first]"))");
})

} // namespace reopened_namespace

namespace nested {

GENPYBIND_MANUAL({
  ::pybind11::exec(R"(track_order("::nested [before Example]"))");
})

struct GENPYBIND(visible) Example {
  GENPYBIND_MANUAL({
    ::pybind11::exec(R"(track_order("::nested::Example [first]"))");
  })

  GENPYBIND_MANUAL({
    ::pybind11::exec(R"(track_order("::nested::Example [second]"))");
  })
};

GENPYBIND_MANUAL({
  ::pybind11::exec(R"(track_order("::nested [after Example]"))");
})

namespace nested {

GENPYBIND_MANUAL({ ::pybind11::exec(R"(track_order("::nested::nested"))"); })

} // namespace nested

} // namespace nested

namespace reopened_namespace {

GENPYBIND_MANUAL({
  ::pybind11::exec(R"(track_order("::reopened_namespace [second]"))");
})

} // namespace reopened_namespace

struct GENPYBIND(visible) Toplevel {
  GENPYBIND_MANUAL({ ::pybind11::exec(R"(track_order("::Toplevel"))"); })
};

namespace reopened_namespace {

GENPYBIND_MANUAL({
  ::pybind11::exec(R"(track_order("::reopened_namespace [third]"))");
})

} // namespace reopened_namespace

GENPYBIND_MANUAL({
  ::pybind11::exec(R"(track_order(":: [before postamble]"))");
})

GENPYBIND(postamble)
GENPYBIND_MANUAL({
  ::pybind11::exec(R"(track_order(":: [first postamble]"))");
})

GENPYBIND(postamble)
GENPYBIND_MANUAL({
  ::pybind11::exec(R"(track_order(":: [second postamble]"))");
})

GENPYBIND_MANUAL({
  ::pybind11::exec(R"(track_order(":: [after postamble]"))");
})
