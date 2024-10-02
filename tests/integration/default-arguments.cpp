// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "default-arguments.h"

const nested::A nested::global_a = nested::A(1234);
const nested::A *const nested::global_a_ptr = &nested::global_a;
nested::twice::C::C() : A(456) {}
const nested::twice::C nested::twice::global_c = nested::twice::C();
const nested::twice::C *const nested::twice::global_c_ptr =
    &nested::twice::global_c;

unsigned function_01(unsigned num, bool, float) { return num; }
void function_02(Example) {}
int nested::function_03(A a) { return a.value; }
int nested::function_04(int value) { return value; }
int nested::function_05(int value) { return value; }
int nested::function_06(int value) { return value; }
int nested::function_07(int value) { return value; }
int nested::function_08(int value) { return value; }
int nested::function_09(int value) { return value; }
int nested::function_10(int value) { return value; }
int nested::function_11(A a) { return a.value; }
int nested::function_12(A a) { return a.value; }
int nested::function_13(A a) { return a.value; }
int nested::function_14(A a) { return a.value; }
// int nested::function_15(const A *a_ptr) { return a->value; }
int function_16(nested::A a) { return a.value; }

int nested::function_17(int value) { return value; }
int nested::function_18(int value) { return value; }
int nested::function_19(int value) { return value; }
int nested::function_20(int value) { return value; }

int nested::function_21(int value) { return value; }
int nested::function_22(int value) { return value; }
int nested::function_23(int value) { return value; }
// int nested::function_24(int value) { return value; }
int nested::function_25(int value) { return value; }

template <typename T> T nested::template_function_01(T value) { return value; }
template <typename T> T nested::template_function_02(T value) { return value; }
template <typename T> T nested::template_function_03(T value) { return value; }
template <typename T> T nested::template_function_04(T value) { return value; }
template <typename T> T nested::template_function_05(T value) { return value; }

template int nested::template_function_01(int);
template int nested::template_function_02(int);
template int nested::template_function_03(int);
template int nested::template_function_04(int);
template int nested::template_function_05(int);
