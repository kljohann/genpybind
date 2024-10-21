// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

// TODO: Add additional mixed-type and reversed-parameters test cases

#include <genpybind/genpybind.h>

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

inline bool operator==(Integer lhs, int rhs) { return lhs.value == rhs; }

inline bool operator<(int lhs, Integer rhs) { return lhs < rhs.value; }

struct GENPYBIND(visible) Templated {
  explicit Templated(int value) : value(value) {}
  int value;

  template <typename T> bool operator==(T other) const {
    return value == other;
  }
};

template <> inline bool Templated::operator==(Templated other) const {
  return value == other.value;
}

extern template bool Templated::operator==(int) const;
extern template bool Templated::operator==(Templated) const;

} // namespace example

namespace exhaustive {

struct GENPYBIND(visible) Exhaustive {
  explicit Exhaustive(int value) : value(value) {}
  int value;
};

struct GENPYBIND(visible) Args {
  Args(int lhs, int rhs) : lhs(lhs), rhs(rhs) {}
  int lhs;
  int rhs;
};

#define BINARY_OPERATOR_TEST(OP_NAME, OP_SPELLING)                             \
  struct GENPYBIND(visible) has_member_##OP_NAME : public Exhaustive {         \
    has_member_##OP_NAME(int value) : Exhaustive(value) {}                     \
    Args operator OP_SPELLING(const has_member_##OP_NAME &other) const {       \
      return Args(value, other.value);                                         \
    }                                                                          \
  };                                                                           \
  struct GENPYBIND(visible) has_nonconst_member_##OP_NAME                      \
      : public Exhaustive {                                                    \
    has_nonconst_member_##OP_NAME(int value) : Exhaustive(value) {}            \
    Args operator OP_SPELLING(const has_nonconst_member_##OP_NAME &other) {    \
      return Args(value, other.value);                                         \
    }                                                                          \
  };                                                                           \
  struct GENPYBIND(visible) has_private_member_##OP_NAME : public Exhaustive { \
    has_private_member_##OP_NAME(int value) : Exhaustive(value) {}             \
                                                                               \
  private:                                                                     \
    Args                                                                       \
    operator OP_SPELLING(const has_private_member_##OP_NAME &other) const {    \
      return Args(value, other.value);                                         \
    }                                                                          \
  };                                                                           \
  struct GENPYBIND(visible) has_hidden_member_##OP_NAME : public Exhaustive {  \
    has_hidden_member_##OP_NAME(int value) : Exhaustive(value) {}              \
    Args operator OP_SPELLING(const has_hidden_member_##OP_NAME &other) const  \
        GENPYBIND(hidden) {                                                    \
      return Args(value, other.value);                                         \
    }                                                                          \
  };                                                                           \
  struct GENPYBIND(visible) has_friend_##OP_NAME : public Exhaustive {         \
    has_friend_##OP_NAME(int value) : Exhaustive(value) {}                     \
    friend Args operator OP_SPELLING(const has_friend_##OP_NAME &lhs,          \
                                     const has_friend_##OP_NAME &rhs) {        \
      return Args(lhs.value, rhs.value);                                       \
    }                                                                          \
  };                                                                           \
  struct GENPYBIND(visible) has_hidden_friend_##OP_NAME : public Exhaustive {  \
    has_hidden_friend_##OP_NAME(int value) : Exhaustive(value) {}              \
    friend Args operator OP_SPELLING(const has_hidden_friend_##OP_NAME &lhs,   \
                                     const has_hidden_friend_##OP_NAME &rhs)   \
        GENPYBIND(hidden) {                                                    \
      return Args(lhs.value, rhs.value);                                       \
    }                                                                          \
  };                                                                           \
  struct GENPYBIND(visible) has_associated_##OP_NAME : public Exhaustive {     \
    has_associated_##OP_NAME(int value) : Exhaustive(value) {}                 \
  };                                                                           \
  inline Args operator OP_SPELLING(const has_associated_##OP_NAME &lhs,        \
                                   const has_associated_##OP_NAME &rhs) {      \
    return Args(lhs.value, rhs.value);                                         \
  }                                                                            \
  struct GENPYBIND(visible) has_hidden_associated_##OP_NAME                    \
      : public Exhaustive {                                                    \
    has_hidden_associated_##OP_NAME(int value) : Exhaustive(value) {}          \
  };                                                                           \
  inline Args operator OP_SPELLING(const has_hidden_associated_##OP_NAME &lhs, \
                                   const has_hidden_associated_##OP_NAME &rhs) \
      GENPYBIND(hidden) {                                                      \
    return Args(lhs.value, rhs.value);                                         \
  }

BINARY_OPERATOR_TEST(add, +)
BINARY_OPERATOR_TEST(sub, -)
BINARY_OPERATOR_TEST(mul, *)
BINARY_OPERATOR_TEST(truediv, /)
BINARY_OPERATOR_TEST(mod, %)
BINARY_OPERATOR_TEST(xor, ^)
BINARY_OPERATOR_TEST(and, &)
BINARY_OPERATOR_TEST(or, |)
BINARY_OPERATOR_TEST(lt, <)
BINARY_OPERATOR_TEST(gt, >)
BINARY_OPERATOR_TEST(iadd, +=)
BINARY_OPERATOR_TEST(isub, -=)
BINARY_OPERATOR_TEST(imul, *=)
BINARY_OPERATOR_TEST(itruediv, /=)
BINARY_OPERATOR_TEST(imod, %=)
BINARY_OPERATOR_TEST(ixor, ^=)
BINARY_OPERATOR_TEST(iand, &=)
BINARY_OPERATOR_TEST(ior, |=)
BINARY_OPERATOR_TEST(lshift, <<)
BINARY_OPERATOR_TEST(rshift, >>)
BINARY_OPERATOR_TEST(ilshift, <<=)
BINARY_OPERATOR_TEST(irshift, >>=)
BINARY_OPERATOR_TEST(eq, ==)
BINARY_OPERATOR_TEST(ne, !=)
BINARY_OPERATOR_TEST(le, <=)
BINARY_OPERATOR_TEST(ge, >=)

#undef BINARY_OPERATOR_TEST

#define UNARY_OPERATOR_TEST(OP_NAME, OP_SPELLING)                              \
  struct GENPYBIND(visible) has_member_##OP_NAME : public Exhaustive {         \
    has_member_##OP_NAME(int value) : Exhaustive(value) {}                     \
    has_member_##OP_NAME operator OP_SPELLING() const {                        \
      return {OP_SPELLING value};                                              \
    }                                                                          \
  };                                                                           \
  struct GENPYBIND(visible) has_nonconst_member_##OP_NAME                      \
      : public Exhaustive {                                                    \
    has_nonconst_member_##OP_NAME(int value) : Exhaustive(value) {}            \
    has_nonconst_member_##OP_NAME operator OP_SPELLING() {                     \
      return {OP_SPELLING value};                                              \
    }                                                                          \
  };                                                                           \
  struct GENPYBIND(visible) has_private_member_##OP_NAME : public Exhaustive { \
    has_private_member_##OP_NAME(int value) : Exhaustive(value) {}             \
                                                                               \
  private:                                                                     \
    has_private_member_##OP_NAME operator OP_SPELLING() const {                \
      return {OP_SPELLING value};                                              \
    }                                                                          \
  };                                                                           \
  struct GENPYBIND(visible) has_hidden_member_##OP_NAME : public Exhaustive {  \
    has_hidden_member_##OP_NAME(int value) : Exhaustive(value) {}              \
    has_hidden_member_##OP_NAME operator OP_SPELLING() const                   \
        GENPYBIND(hidden) {                                                    \
      return {OP_SPELLING value};                                              \
    }                                                                          \
  };                                                                           \
  struct GENPYBIND(visible) has_friend_##OP_NAME : public Exhaustive {         \
    has_friend_##OP_NAME(int value) : Exhaustive(value) {}                     \
    friend has_friend_##OP_NAME                                                \
    operator OP_SPELLING(const has_friend_##OP_NAME &lhs) {                    \
      return {OP_SPELLING lhs.value};                                          \
    }                                                                          \
  };                                                                           \
  struct GENPYBIND(visible) has_hidden_friend_##OP_NAME : public Exhaustive {  \
    has_hidden_friend_##OP_NAME(int value) : Exhaustive(value) {}              \
    friend has_hidden_friend_##OP_NAME                                         \
    operator OP_SPELLING(const has_hidden_friend_##OP_NAME &lhs)               \
        GENPYBIND(hidden) {                                                    \
      return {OP_SPELLING lhs.value};                                          \
    }                                                                          \
  };                                                                           \
  struct GENPYBIND(visible) has_associated_##OP_NAME : public Exhaustive {     \
    has_associated_##OP_NAME(int value) : Exhaustive(value) {}                 \
  };                                                                           \
  inline has_associated_##OP_NAME operator OP_SPELLING(                        \
      const has_associated_##OP_NAME &lhs) {                                   \
    return {OP_SPELLING lhs.value};                                            \
  }                                                                            \
  struct GENPYBIND(visible) has_hidden_associated_##OP_NAME                    \
      : public Exhaustive {                                                    \
    has_hidden_associated_##OP_NAME(int value) : Exhaustive(value) {}          \
  };                                                                           \
  inline has_hidden_associated_##OP_NAME operator OP_SPELLING(                 \
      const has_hidden_associated_##OP_NAME &lhs) GENPYBIND(hidden) {          \
    return {OP_SPELLING lhs.value};                                            \
  }

UNARY_OPERATOR_TEST(pos, +)
UNARY_OPERATOR_TEST(neg, -)
UNARY_OPERATOR_TEST(invert, ~)

#undef UNARY_OPERATOR_TEST

} // namespace exhaustive
