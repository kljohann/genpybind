// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <optional>

// clang-format off
#ifdef __GENPYBIND__
#pragma genpybind include<pybind11/stl.h>
#endif // __GENPYBIND__
// clang-format on
#include <genpybind/genpybind.h>

std::optional<int> example(std::optional<int> value = std::nullopt)
    GENPYBIND(visible);
