// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "readme.h"

using namespace readme;

int Example::calculate(Flavor flavor) const {
  return flavor == Flavor::bland ? m_value : -m_value;
}

int Example::getSomething() const { return m_value; }

void Example::setSomething(int value) { m_value = value; }
