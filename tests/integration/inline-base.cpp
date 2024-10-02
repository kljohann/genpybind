// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "inline-base.h"

bool Base::from_base() const { return true; }

bool Base::hidden() const { return false; }

bool InlineBase::hidden() const { return true; }

bool HideBase::hidden() const { return true; }

bool Other::from_other() const { return true; }

bool Derived::from_derived() const { return true; }

int ConflictOne::number() const { return 1; }

int ConflictTwo::number() const { return 2; }

bool nested::NestedBase::from_nested_base() const { return true; }
