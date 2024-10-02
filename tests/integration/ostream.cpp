// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "ostream.h"

std::string Example::to_string() const { return std::to_string(value); }

std::ostream &operator<<(std::ostream &os, const Example &example) {
  return os << "Example(" << example.value << ")";
}

std::ostream &operator<<(std::ostream &os, const ExposedAsStr & /*value*/) {
  return os << "human readable";
}

std::ostream &operator<<(std::ostream &os, const ExposedAsRepr & /*value*/) {
  return os << "ExposedAsRepr()";
}

std::ostream &RedHerring::operator<<(std::ostream &os) const {
  return os << "strange";
}

std::ostream &AnotherRedHerring::operator<<(std::ostream &os) const {
  return os << "strange";
}
