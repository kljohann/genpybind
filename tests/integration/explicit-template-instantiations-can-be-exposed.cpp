#include "explicit-template-instantiations-can-be-exposed.h"

template struct ExposeSomeInstantiations<int>;
template struct ExposeAll<int>;
