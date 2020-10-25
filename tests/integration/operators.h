#pragma once

#include "genpybind.h"

namespace example {

struct GENPYBIND(visible) Other {};

struct GENPYBIND(visible) Number {
  explicit Number(int value) : value(value) {}
  int value;

  Number &operator+=(int) = delete;
  friend Number &operator-=(const Number &, int) = delete;

  bool operator==(Number other) const { return value == other.value; }
  bool operator>(int other) const { return value > other; }

  friend bool operator<(const Number &lhs, const Number &rhs) {
    return lhs.value < rhs.value;
  }

  friend bool operator==(Number lhs, int rhs) { return lhs.value == rhs; }

  friend bool operator==(Other, Number) { return false; }

  friend bool operator>(int lhs, Number rhs) { return lhs > rhs.value; }

  friend bool operator==(example::Number, bool) { return false; }

  friend bool operator==(Other, Other) { return true; }
};

struct GENPYBIND(visible) Integer {
  explicit Integer(int value) : value(value) {}
  int value;

  bool operator==(Integer other) const { return value == other.value; }

  friend bool operator<(const Integer &lhs, const Integer &rhs) {
    return lhs.value < rhs.value;
  }

private:
  bool operator==(Number other) const { return value == other.value; }
};

bool operator==(Integer lhs, int rhs) { return lhs.value == rhs; }

bool operator<(int lhs, Integer rhs) { return lhs < rhs.value; }

struct GENPYBIND(visible) Templated {
  explicit Templated(int value) : value(value) {}
  int value;

  template <typename T> bool operator==(T other) const {
    return value == other;
  }
};

template <> bool Templated::operator==(Templated other) const {
  return value == other.value;
}

extern template bool Templated::operator==(int) const;
extern template bool Templated::operator==(Templated) const;

} // namespace example
