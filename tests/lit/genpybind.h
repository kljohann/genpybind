// RUN: true
#pragma once

#ifdef __GENPYBIND__
#define GENPYBIND(...) __attribute__((annotate("◊" #__VA_ARGS__)))
#else
#define GENPYBIND(...)
#endif // __GENPYBIND__
