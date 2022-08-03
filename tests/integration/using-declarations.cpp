#include "using-declarations.h"

template int TplBase::neg(int);

template <typename T> T TplBase::neg(T value) { return -value; }
