// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "base-classes.h"

bool Abstract::static_method() { return true; }

int Abstract::defined_in_base() const { return 5; }
bool Abstract::overridden() const { return false; }

int DerivedFromAbstract::abstract() const { return 7; }
bool DerivedFromAbstract::overridden() const { return true; }
