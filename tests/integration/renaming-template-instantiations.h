// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

template <int Value> struct GENPYBIND(expose_as(_Renamed)) Renamed {};
// NOTE: All instantiations will be exposed as `_Renamed` (i.e., have the exact
// same name).
template struct Renamed<123>;
// TODO: Make it possible to change a single name, as follows.
// (Unfortunately, the `ClassTemplateSpecializationDecl` has both
// `AnnotateAttr`s: `expose_as(_RenamedZero)` and `expose_as(_Renamed)`, in
// that order.  Neither isImplicit nor isInherited returns true for that attr.)
template struct GENPYBIND(expose_as(_RenamedZero)) Renamed<0>;
