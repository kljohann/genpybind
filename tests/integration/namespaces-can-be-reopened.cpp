#include "namespaces-can-be-reopened.h"

bool example::hidden_in_first() { return true; }
bool example::hidden_in_second() { return true; }
bool example::visible_in_first() { return true; }
bool example::visible_in_second() { return true; }
bool submodule::visible_in_first() { return true; }
bool submodule::visible_in_second() { return true; }
bool nothing_exposed_in_original_ns::and_later() { return true; }
