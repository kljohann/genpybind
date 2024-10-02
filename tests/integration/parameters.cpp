// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "parameters.h"

void parameter_names(bool, int, unsigned, double) {}

void missing_names(bool, bool, bool, bool) {}

int return_second(int, int second) { return second; }

Example::Example(bool, bool) {}

void Example::method(bool, bool) const {}

void TakesPointers::accepts_none(Example *) {}
void TakesPointers::required(Example *) {}
void TakesPointers::required_second(Example *, Example *) {}
void TakesPointers::required_both(Example *, Example *) {}

void accepts_none(Example *) {}
void required(Example *) {}
void required_second(Example *, Example *) {}
void required_both(Example *, Example *) {}

bool TakesDouble::overload_is_double(int) const { return false; }
bool TakesDouble::overload_is_double(double) const { return true; }
double TakesDouble::normal(double value) const { return value; }
double TakesDouble::noconvert(double value) const { return value; }
double TakesDouble::noconvert_second(double first, double second) const {
  return first + second;
}
double TakesDouble::noconvert_both(double first, double second) const {
  return first + second;
}

bool overload_is_double(int) { return false; }
bool overload_is_double(double) { return true; }
double normal(double value) { return value; }
double noconvert(double value) { return value; }
double noconvert_second(double first, double second) { return first + second; }
double noconvert_both(double first, double second) { return first + second; }
