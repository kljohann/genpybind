// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

struct GENPYBIND(visible) Example {
  GENPYBIND(getter_for(value))
  int getValue() const;

  GENPYBIND(setter_for(value))
  void setValue(int value);

  GENPYBIND(getter_for(readonly))
  bool getReadonly() const;

  // NOTE: This does not actually work, since the default argument cannot be
  // propagated to pybind11.
  GENPYBIND(getter_for(combined), setter_for(combined))
  int strange(const int *value = nullptr);

  GENPYBIND(getter_for(multi_1), getter_for(multi_2))
  int getMulti() const;

  GENPYBIND(setter_for(multi_1), setter_for(multi_2))
  void setMulti(int value);

  int value_ = 0;
  int combined_ = 0;
  int multi_ = 0;
};
