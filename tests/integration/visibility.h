// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

struct NoAnnotations {};
struct GENPYBIND() NoArguments {};
struct GENPYBIND(visible) ExplicitlyVisible {};
struct GENPYBIND(dynamic_attr) ImplicitlyVisible {};

void no_annotations();
void no_arguments() GENPYBIND();
void explicitly_visible() GENPYBIND(visible);
void implicitly_visible(int value) GENPYBIND(noconvert(value));
