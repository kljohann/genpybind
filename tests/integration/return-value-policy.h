#pragma once

#include "genpybind.h"

struct GENPYBIND(visible) Number {
  int value = 0;
};

struct GENPYBIND(visible) Example {
  Number number;

  GENPYBIND(getter_for(value))
  int value() const;

  Number &reference();

  const Number &constReference() const;

  GENPYBIND(return_value_policy(copy))
  Number &referenceAsCopy();

  GENPYBIND(return_value_policy(copy))
  const Number &constReferenceAsCopy() const;

  GENPYBIND(return_value_policy(reference_internal))
  Number &referenceAsInternalReference();

  GENPYBIND(return_value_policy(reference_internal))
  const Number &constReferenceAsInternalReference() const;
};
