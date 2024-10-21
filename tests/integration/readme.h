// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

namespace readme GENPYBIND(visible) {

/// Describes how the output will taste.
enum class Flavor {
  /// Like you would expect.
  bland,
  /// It tastes different.
  fruity,
};

/// A contrived example.
class Example {
public:
  static constexpr int GENPYBIND(hidden) not_exposed = 10;

  /// Do a complicated calculation.
  int calculate(Flavor flavor = Flavor::fruity) const;

  GENPYBIND(getter_for(something))
  int getSomething() const;

  GENPYBIND(setter_for(something))
  void setSomething(int value);

private:
  int m_value = 0;
};

} // namespace readme
