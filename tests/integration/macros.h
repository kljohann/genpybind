#pragma once

#include "genpybind.h"

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define EXPOSE_AS_FROM_MACRO(name)                                             \
  struct GENPYBIND(expose_as(CONCAT(name, Exposed))) name {};

EXPOSE_AS_FROM_MACRO(One)
EXPOSE_AS_FROM_MACRO(Two)
