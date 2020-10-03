#pragma once

#include "genpybind.h"

struct GENPYBIND(visible) root {};

struct GENPYBIND(visible) derived : public root {};

GENPYBIND(visible)
void context() {}
