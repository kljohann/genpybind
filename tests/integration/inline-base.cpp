#include "inline-base.h"

bool Base::from_base() const { return true; }
bool Base::hidden() const { return false; }

bool InlineBase::hidden() const { return true; }

bool Other::from_other() const { return true; }

bool Derived::from_derived() const { return true; }

int NumberOne::number() const { return 1; }

int NumberTwo::number() const { return 2; }

bool nested::NestedBase::from_nested_base() const { return true; }
