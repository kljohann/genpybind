#include "fields-and-variables.h"

int global_variable = 123;
const int global_const_variable = 456;

int Example::static_variable = 1;
const int Example::static_const_variable;
constexpr int Example::static_constexpr_variable;
int Example::readonly_static_variable = 1;
const int Example::readonly_static_const_variable;
