#pragma once

#include "genpybind.h"

namespace deeply {
namespace nested {
struct GENPYBIND(visible) Base {};

template <typename T> struct GENPYBIND(visible) CRTP {};

struct Hidden {};
} // namespace nested
} // namespace deeply

struct GENPYBIND(visible) Derived : public deeply::nested::Base {};

struct GENPYBIND(visible) DerivedCRTP
    : public deeply::nested::CRTP<DerivedCRTP> {};

struct GENPYBIND(visible) DerivedFromHidden : public deeply::nested::Hidden {};
