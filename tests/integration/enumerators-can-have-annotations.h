// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

enum class GENPYBIND(visible) Example {
  Hidden GENPYBIND(hidden),
  Original GENPYBIND(expose_as(Renamed)),
  NotAnnotated,
};
