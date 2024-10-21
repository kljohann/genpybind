// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <type_traits>

#ifdef __GENPYBIND__

#define GENPYBIND__PRIVATE_CONCAT_EXPANDED__(x, y) x##y
#define GENPYBIND__PRIVATE_CONCAT__(x, y)                                      \
  GENPYBIND__PRIVATE_CONCAT_EXPANDED__(x, y)
#define GENPYBIND__PRIVATE_UNIQUE__(x)                                         \
  GENPYBIND__PRIVATE_CONCAT__(x, __COUNTER__)
#define GENPYBIND__PRIVATE_STRINGIZE__(...) #__VA_ARGS__

#define GENPYBIND(...)                                                         \
  __attribute__((annotate("◊" GENPYBIND__PRIVATE_STRINGIZE__(__VA_ARGS__))))
#define GENPYBIND_MANUAL(...)                                                  \
  static constexpr auto __attribute__((unused)) GENPYBIND(manual)              \
      GENPYBIND__PRIVATE_UNIQUE__(genpybind_) = [](auto &parent) __VA_ARGS__;
#define GENPYBIND_PARENT_TYPE                                                  \
  typename std::remove_reference_t<decltype(parent)>::type

#else // __GENPYBIND__

#define GENPYBIND(...)
#define GENPYBIND_MANUAL(...)

#endif // __GENPYBIND__
