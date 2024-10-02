// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "methods.h"

Other::operator Example() const { return {}; }

void consumes_other(Other) {}

int Example::public_method() { return 5; }
int Example::public_const_method() const { return 123; }
int Example::old_name() const { return 42; }
int Example::overloaded(int value) const { return value; }
double Example::overloaded(double value) const { return value; }
void Example::hidden_method() {}
Example Example::static_method() { return {}; }
void Example::private_method() {}

Example::operator int() const { return 123; }

Example::operator bool() const { return true; }
Example::operator Other() const { return {}; }

int ExoticMemberFunctions::function() { return 42; }
int ExoticMemberFunctions::other_function() const { return 123; }
