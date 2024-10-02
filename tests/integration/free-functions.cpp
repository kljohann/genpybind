// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "free-functions.h"

bool invert(bool value) { return !value; }

int add(int lhs, int rhs) { return lhs + rhs; }
double add(double lhs, double rhs) { return lhs + rhs; }

int old_name() { return 123; }

bool example::visible() { return true; }
bool example::hidden() { return true; }
bool not_exposed() { return true; }

int missing_param_names(int lhs, int rhs) { return lhs + rhs; }
