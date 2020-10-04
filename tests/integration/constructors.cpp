#include "constructors.h"

Example::Example(int value, bool flag) : value_(value), flag_(flag) {}
Example::Example(int value) : value_(value), flag_(false) {}
int Example::value() const { return value_; }
bool Example::flag() const { return flag_; }

AcceptsNone::AcceptsNone(const Example *) {}
RejectsNone::RejectsNone(const Example *) {}

int accepts_implicit(Implicit value) { return value.value(); }
int noconvert_implicit(Implicit value) { return value.value(); }
int accepts_implicit_ref(ImplicitRef value) { return value.value(); }
int accepts_implicit_ptr(ImplicitPtr value) { return value.value(); }
