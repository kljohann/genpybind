#pragma once

#include "genpybind/annotations/literal_value.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

#include <iosfwd>
#include <utility>

namespace genpybind {
namespace annotations {

class AnnotationKind {
public:
  enum Kind {
#define ANNOTATION_KIND(Enum, Spelling) Enum,
#include "genpybind/annotations/annotations.def"
  };

  AnnotationKind(Kind kind) : kind(kind) {}

  Kind value() const { return kind; }
  void print(llvm::raw_ostream &os) const;

  friend bool operator==(const AnnotationKind &left,
                         const AnnotationKind &right) {
    return left.value() == right.value();
  }
  friend bool operator!=(const AnnotationKind &left,
                         const AnnotationKind &right) {
    return !(left == right);
  }

private:
  Kind kind;
};

llvm::StringRef toString(AnnotationKind kind);

class Annotation {
public:
  using Arguments = llvm::SmallVector<LiteralValue, 1>;

  Annotation(AnnotationKind kind) : kind(kind), arguments() {}
  Annotation(AnnotationKind kind, Arguments &&arguments)
      : kind(kind), arguments(std::move(arguments)) {}

  AnnotationKind getKind() const { return kind; }
  const Arguments &getArguments() const { return arguments; }

  void print(llvm::raw_ostream &os) const;

private:
  AnnotationKind kind;
  Arguments arguments;
};

void PrintTo(const AnnotationKind &kind, std::ostream *os);
void PrintTo(const Annotation &annotation, std::ostream *os);

} // namespace annotations
} // namespace genpybind
