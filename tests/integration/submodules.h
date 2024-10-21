// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

struct GENPYBIND(visible) X {};

namespace one GENPYBIND(module) {
using X GENPYBIND(visible) = ::X;
}

namespace two GENPYBIND(module(deux)) {
using X GENPYBIND(visible) = ::X;
}

namespace three GENPYBIND(expose_as(drei), module) {
using X GENPYBIND(visible) = ::X;
}

namespace four GENPYBIND(expose_as(vier), module(quatre)) {
using X GENPYBIND(visible) = ::X;
}

namespace five GENPYBIND(module(fuenf), expose_as(cinq)) {
using X GENPYBIND(visible) = ::X;
}

namespace six GENPYBIND(module(sechs), module(seis)) {
using X GENPYBIND(visible) = ::X;
}

namespace reopened GENPYBIND(module) {}

namespace reopened GENPYBIND(module) {
struct GENPYBIND(visible) Y {};
using X GENPYBIND(visible) = ::X;
}

namespace reopened_2 GENPYBIND(module) {
struct GENPYBIND(visible) Y {};
using X GENPYBIND(visible) = ::X;
}

namespace reopened_2 GENPYBIND(module) {
struct GENPYBIND(visible) Z {};
using X_2 GENPYBIND(visible) = ::X;
}
