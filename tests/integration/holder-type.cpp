// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "holder-type.h"

std::shared_ptr<Shared> Shared::clone() { return shared_from_this(); }
Holder::Holder() : shared(std::make_shared<Shared>()) {}
std::shared_ptr<Shared> Holder::getShared() { return shared; }
long Holder::uses() const { return shared.use_count(); }
