#pragma once

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

#include <iosfwd>
#include <string>

namespace genpybind {
namespace annotations {

/// A literal value used as argument to one of the annotations.
///
/// Internally it is implemented as a tagged union that can hold
/// a string, an unsigned integer, a boolean, or the special values
/// `default` and `nothing` (i.e. contains no value).
class LiteralValue {
  enum class Kind {
    Nothing,
    String,
    Unsigned,
    Boolean,
    Default,
  } kind;
  union {
    std::string *string;
    unsigned integer;
    bool boolean;
  };

public:
  ~LiteralValue();
  LiteralValue() : kind(Kind::Nothing), string(nullptr) {}
  static LiteralValue createString(llvm::StringRef value);
  static LiteralValue createUnsigned(unsigned value);
  static LiteralValue createBoolean(bool value);
  static LiteralValue createDefault();

  LiteralValue(const LiteralValue &other);
  LiteralValue &operator=(const LiteralValue &other);
  LiteralValue(LiteralValue &&other) noexcept;
  LiteralValue &operator=(LiteralValue &&other) noexcept;

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

void PrintTo(const LiteralValue &value, std::ostream *os);

} // namespace annotations
} // namespace genpybind
