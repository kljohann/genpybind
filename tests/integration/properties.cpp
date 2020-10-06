#include "properties.h"

int Example::getValue() const { return value_; }
void Example::setValue(int value) { value_ = value; }

bool Example::getReadonly() const { return true; }

int Example::strange(const int *value) {
  if (value != nullptr)
    combined_ = *value;
  return combined_;
}

int Example::getMulti() const { return multi_; }
void Example::setMulti(int value) { multi_ = value; }
