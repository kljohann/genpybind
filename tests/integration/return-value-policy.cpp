#include "return-value-policy.h"

int Example::value() const { return number.value; }

Number &Example::reference() { return number; }

const Number &Example::constReference() const { return number; }

Number &Example::referenceAsCopy() { return number; }

const Number &Example::constReferenceAsCopy() const { return number; }

Number &Example::referenceAsInternalReference() { return number; }

const Number &Example::constReferenceAsInternalReference() const {
  return number;
}
