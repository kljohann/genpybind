#include "operators.h"

namespace example {
template bool Templated::operator==(int) const;
template bool Templated::operator==(Templated) const;
} // namespace example
