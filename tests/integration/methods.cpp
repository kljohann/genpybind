#include "methods.h"

int Example::public_method() { return 5; }
int Example::public_const_method() const { return 123; }
int Example::old_name() const { return 42; }
int Example::overloaded(int value) const { return value; }
double Example::overloaded(double value) const { return value; }
void Example::hidden_method() {}
Example Example::static_method() { return {}; }
void Example::private_method() {}

int ExoticMemberFunctions::function() { return 42; }
int ExoticMemberFunctions::other_function() const { return 123; }
