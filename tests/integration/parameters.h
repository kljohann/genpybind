// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

GENPYBIND(visible)
void parameter_names(bool are, int part, unsigned of, double the_signature);

GENPYBIND(visible)
void missing_names(bool, bool not_a, bool, bool problem);

GENPYBIND(visible)
int return_second(int first, int second);

struct GENPYBIND(visible) Example {
  Example() = default;
  Example(bool also, bool here);

  void method(bool it, bool works) const;
};

struct GENPYBIND(visible) TakesPointers {
  void accepts_none(Example *example);

  GENPYBIND(required(example))
  void required(Example *example);

  GENPYBIND(required(second))
  void required_second(Example *first, Example *second);

  GENPYBIND(required(first, second))
  void required_both(Example *first, Example *second);
};

GENPYBIND(visible)
void accepts_none(Example *example);

GENPYBIND(required(example))
void required(Example *example);

GENPYBIND(required(second))
void required_second(Example *first, Example *second);

GENPYBIND(required(first, second))
void required_both(Example *first, Example *second);

struct GENPYBIND(visible) TakesDouble {
  bool overload_is_double(int value) const;
  bool overload_is_double(double value) const;

  double normal(double value) const;

  GENPYBIND(noconvert(value))
  double noconvert(double value) const;

  GENPYBIND(noconvert(second))
  double noconvert_second(double first, double second) const;

  GENPYBIND(noconvert(first, second))
  double noconvert_both(double first, double second) const;
};

GENPYBIND(visible)
bool overload_is_double(int value);

GENPYBIND(visible)
bool overload_is_double(double value);

GENPYBIND(visible)
double normal(double value);

GENPYBIND(noconvert(value))
double noconvert(double value);

GENPYBIND(noconvert(second))
double noconvert_second(double first, double second);

GENPYBIND(noconvert(first, second))
double noconvert_both(double first, double second);
