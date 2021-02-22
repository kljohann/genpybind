#include "ostream.h"

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
