// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

struct GENPYBIND(visible) Example {};

unsigned function_01(unsigned num = 123U, bool flag = true,
                     float value = 123.45F) GENPYBIND(visible);
void function_02(Example example = Example()) GENPYBIND(visible);

namespace nested {

struct GENPYBIND(visible) A {
  A(int value = 0) : value(value) {}

  int value;
};
struct GENPYBIND(visible) B : A {};

extern const A global_a;
extern const A *const global_a_ptr;

namespace twice {
struct GENPYBIND(visible) C : A {
  C();
};
extern const C global_c;
extern const C *const global_c_ptr;
} // namespace twice

int function_03(A a = A(3)) GENPYBIND(visible);
int function_04(int value = A(4).value) GENPYBIND(visible);
int function_05(int value = global_a.value) GENPYBIND(visible);
int function_06(int value = global_a_ptr->value) GENPYBIND(visible);
int function_07(int value = twice::global_c.value) GENPYBIND(visible);
int function_08(int value = twice::global_c_ptr->value) GENPYBIND(visible);
int function_09(int value = twice::global_c.twice::C::value) GENPYBIND(visible);
int function_10(int value = twice::global_c_ptr->twice::C::value)
    GENPYBIND(visible);

int function_11(A a = (A)B()) GENPYBIND(visible);
int function_12(A a = static_cast<A>(B())) GENPYBIND(visible);
int function_13(A a = {5}) GENPYBIND(visible);
int function_14(A a = A{5}) GENPYBIND(visible);
// int function_15(const A *a_ptr = &global_a) GENPYBIND(visible);

} // namespace nested

int function_16(nested::A a = nested::A{6}) GENPYBIND(visible);

namespace nested {
enum class Enum { A, B, C };
}

// TODO: Non-type template parameters fail to be fully qualified.
template <typename T> struct Type {
  static constexpr int value = 123;
};
template <int N> struct Integral {
  static constexpr int value = N;
};
template <nested::Enum E> struct Enumeration {
  static constexpr int value = 123;
};
template <const nested::A &A> struct Reference {
  static constexpr int value = 123;
};
template <const nested::A *A> struct Pointer {
  static constexpr int value = 123;
};

namespace nested {
inline constexpr int N = 123;
constexpr int count() { return 123; }
struct Holder {
  static constexpr int N = 123;
  static constexpr int count() { return 123; }
};

int function_17(int value = Type<A>::value) GENPYBIND(visible);
int function_18(int value = Integral<123>::value) GENPYBIND(visible);
int function_19(int value = Integral<N>::value) GENPYBIND(visible);
int function_20(int value = Integral<count()>::value) GENPYBIND(visible);
int function_21(int value = Integral<Holder::N>::value) GENPYBIND(visible);
int function_22(int value = Integral<Holder::count()>::value)
    GENPYBIND(visible);
int function_23(int value = Enumeration<Enum::A>::value) GENPYBIND(visible);
// int function_24(int value = Reference<global_a>::value) GENPYBIND(visible);
int function_25(int value = Pointer<&global_a>::value) GENPYBIND(visible);

template <typename T> T template_function_01(T value = 123) GENPYBIND(visible);
extern template int template_function_01(int);
template <typename T> T template_function_02(T value = N) GENPYBIND(visible);
extern template int template_function_02(int);
template <typename T>
T template_function_03(T value = count()) GENPYBIND(visible);
extern template int template_function_03(int);
template <typename T>
T template_function_04(T value = Holder::N) GENPYBIND(visible);
extern template int template_function_04(int);
template <typename T>
T template_function_05(T value = Holder::N) GENPYBIND(visible);
extern template int template_function_05(int);
} // namespace nested

template <typename T> struct GENPYBIND(visible) Template {
  int member_function(int value = 123) const { return value; }
};

template struct Template<Example>;
