// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "using-declarations.h"

template int TplBase::neg(int);

template <typename T> T TplBase::neg(T value) { return -value; }
