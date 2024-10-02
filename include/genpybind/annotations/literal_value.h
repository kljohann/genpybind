// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <llvm/ADT/StringRef.h>

#include <iosfwd>
#include <string>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace genpybind {
namespace annotations {

/// A literal value used as argument to one of the annotations.
///
/// Internally it is implemented as a tagged union that can hold
/// a string, an unsigned integer, a boolean, or the special values
/// `default` and `nothing` (i.e. contains no value).
class LiteralValue {
public:
  enum class Kind {
    Nothing,
    String,
    Unsigned,
    Boolean,
    Default,
  };

private:
  Kind kind;
  union {
    std::string *string = nullptr;
    unsigned integer;
    bool boolean;
  };

public:
  ~LiteralValue();
  LiteralValue() : kind(Kind::Nothing) {}
  static LiteralValue createString(llvm::StringRef value);
  static LiteralValue createUnsigned(unsigned value);
  static LiteralValue createBoolean(bool value);
  static LiteralValue createDefault();

  LiteralValue(const LiteralValue &other);
  LiteralValue &operator=(const LiteralValue &other);
  LiteralValue(LiteralValue &&other) noexcept;
  LiteralValue &operator=(LiteralValue &&other) noexcept;

  bool isa(Kind other) const { return kind == other; }
  bool isNothing() const { return kind == Kind::Nothing; }
  bool isString() const { return kind == Kind::String; }
  bool isUnsigned() const { return kind == Kind::Unsigned; }
  bool isBoolean() const { return kind == Kind::Boolean; }
  bool isDefault() const { return kind == Kind::Default; }
  bool hasValue() const { return kind != Kind::Nothing; }
  explicit operator bool() const { return hasValue(); }

  void setNothing();
  void setString(llvm::StringRef value);
  void setUnsigned(unsigned value);
  void setBoolean(bool value);
  void setDefault();

  const std::string &getString() const;
  unsigned getUnsigned() const;
  bool getBoolean() const;

  void print(llvm::raw_ostream &os) const;

  bool operator==(const LiteralValue &other) const;
  bool operator!=(const LiteralValue &other) const { return !(*this == other); }
};

std::string toString(const LiteralValue &value);
void PrintTo(const LiteralValue &value, std::ostream *os);

} // namespace annotations
} // namespace genpybind
