// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "constructors.h"

Example::Example(int value, bool flag) : value(value), flag(flag) {}
Example::Example(int value) : value(value), flag(false) {}

AcceptsNone::AcceptsNone(const Example *) {}
RejectsNone::RejectsNone(const Example *) {}

int accepts_implicit(Implicit value) { return value.value; }
int noconvert_implicit(Implicit value) { return value.value; }
int accepts_implicit_ref(ImplicitRef value) { return value.value; }
int accepts_implicit_ptr(ImplicitPtr value) { return value.value; }
