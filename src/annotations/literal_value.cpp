// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/annotations/literal_value.h"

#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>
#include <utility>

using namespace genpybind::annotations;

LiteralValue::~LiteralValue() {
  if (isString())
    delete string;
}

LiteralValue LiteralValue::createString(llvm::StringRef value) {
  LiteralValue result;
  result.setString(value);
  return result;
}

LiteralValue LiteralValue::createUnsigned(unsigned value) {
  LiteralValue result;
  result.setUnsigned(value);
  return result;
}

LiteralValue LiteralValue::createBoolean(bool value) {
  LiteralValue result;
  result.setBoolean(value);
  return result;
}

LiteralValue LiteralValue::createDefault() {
  LiteralValue result;
  result.setDefault();
  return result;
}

LiteralValue::LiteralValue(const LiteralValue &other) : kind(Kind::Nothing) {
  *this = other;
}

LiteralValue &LiteralValue::operator=(const LiteralValue &other) {
  if (this == &other)
    return *this;

  setNothing();
  switch (other.kind) {
  case Kind::Nothing:
    break;
  case Kind::String:
    setString(other.getString());
    break;
  case Kind::Unsigned:
    setUnsigned(other.getUnsigned());
    break;
  case Kind::Boolean:
    setBoolean(other.getBoolean());
    break;
  case Kind::Default:
    setDefault();
    break;
  }
  return *this;
}

LiteralValue::LiteralValue(LiteralValue &&other) noexcept
    : kind(Kind::Nothing) {
  if (other.isString()) {
    kind = Kind::String;
    std::swap(string, other.string);
    other.kind = Kind::Nothing;
  } else {
    *this = other;
  }
}

LiteralValue &LiteralValue::operator=(LiteralValue &&other) noexcept {
  if (other.isString()) {
    if (!isString()) {
      kind = Kind::String;
      string = nullptr;
    }
    std::swap(string, other.string);
  } else {
    *this = other;
  }
  return *this;
}

void LiteralValue::setNothing() {
  if (isString())
    delete string;
  kind = Kind::Nothing;
  string = nullptr;
}

void LiteralValue::setString(llvm::StringRef value) {
  setNothing();
  kind = Kind::String;
  string = new std::string(value);
}

void LiteralValue::setUnsigned(unsigned value) {
  setNothing();
  kind = Kind::Unsigned;
  integer = value;
}

void LiteralValue::setBoolean(bool value) {
  setNothing();
  kind = Kind::Boolean;
  boolean = value;
}

void LiteralValue::setDefault() {
  setNothing();
  kind = Kind::Default;
}

const std::string &LiteralValue::getString() const {
  assert(isString());
  return *string;
}

unsigned LiteralValue::getUnsigned() const {
  assert(isUnsigned());
  return integer;
}

bool LiteralValue::getBoolean() const {
  assert(isBoolean());
  return boolean;
}

void LiteralValue::print(llvm::raw_ostream &os) const {
  switch (kind) {
  case Kind::String:
    os << '"';
    os.write_escaped(getString());
    os << '"';
    break;
  case Kind::Unsigned:
    os << getUnsigned();
    break;
  case Kind::Boolean:
    os << (getBoolean() ? "true" : "false");
    break;
  case Kind::Nothing:
    os << "nothing";
    break;
  case Kind::Default:
    os << "default";
    break;
  }
}

bool LiteralValue::operator==(const LiteralValue &other) const {
  if (kind != other.kind)
    return false;
  switch (kind) {
  case Kind::String:
    return getString() == other.getString();
  case Kind::Unsigned:
    return getUnsigned() == other.getUnsigned();
  case Kind::Boolean:
    return getBoolean() == other.getBoolean();
  case Kind::Nothing:
  case Kind::Default:
    return true;
  }
  llvm_unreachable("Unknown literal value kind.");
}

std::string genpybind::annotations::toString(const LiteralValue &value) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  value.print(stream);
  return stream.str();
}

// NOLINTNEXTLINE(readability-identifier-naming)
void genpybind::annotations::PrintTo(const LiteralValue &value,
                                     std::ostream *os) {
  llvm::raw_os_ostream ostream(*os);
  value.print(ostream);
}
