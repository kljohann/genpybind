#pragma once

#include "genpybind.h"

/// \brief Expose instantiations of a function template.
template <typename T> T identity(const T &value) GENPYBIND(visible);

/// This always returns the same number.
template <int N> int constant() GENPYBIND(visible);

extern template int identity<int>(const int &);
extern template double identity<double>(const double &);

extern template int constant<42>();

/// It's not possible to call a specific instantiation from Python as it
/// does not accept arguments.
template <int N> int oops() GENPYBIND(visible);

extern template int oops<123>();
extern template int oops<321>();

/// But an instantiation can be given a distinct name.
extern template GENPYBIND(expose_as(oops_42)) int oops<42>();
