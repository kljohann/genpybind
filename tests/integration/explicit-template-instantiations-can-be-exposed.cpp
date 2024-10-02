// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "explicit-template-instantiations-can-be-exposed.h"

template struct ExposeSomeInstantiations<int>;
template struct ExposeAll<int>;
