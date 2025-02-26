// SPDX-FileCopyrightText: 2025 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "explicit-object-parameters.h"

void Plain::set_double(this Plain &self, int value) { self.value = 2 * value; }

int Plain::get_double(this Plain const &self) { return 2 * self.value; }

template int Base::get_value(const ExplInstUsing &);
template void Base::set_value(ExplInstUsing &, int value);

template int Base::get_value(const ExplInstInlined &);
template void Base::set_value(ExplInstInlined &, int value);

template int VisibleBase::to_kind(this const One &self);
template int VisibleBase::to_kind(this const Two &self);
