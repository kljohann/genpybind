#pragma once

#include "genpybind.h"

struct Derived;

// Even if this declaration is processed earlier, the base class has to be
// exposed first in order to avoid "referenced unknown base" errors.
using Early GENPYBIND(expose_here, expose_as(Derived)) = Derived;

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
