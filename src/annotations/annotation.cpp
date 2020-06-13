#include "genpybind/annotations/annotation.h"

#include <llvm/Support/raw_os_ostream.h>

namespace genpybind {
namespace annotations {

llvm::StringRef toString(AnnotationKind kind) {
  switch (kind.value()) {
#define ANNOTATION_KIND(Enum, Spelling)                                        \
  case AnnotationKind::Kind::Enum:                                             \
    return #Spelling;
#include "genpybind/annotations/annotations.def"
  }
  llvm_unreachable("Unknown annotation kind.");
}

void AnnotationKind::print(llvm::raw_ostream &os) const {
  os << toString(*this);
}

void Annotation::print(llvm::raw_ostream &os) const {
  kind.print(os);
  os << '(';
  bool comma = false;
  for (const auto &arg : arguments) {
    if (comma)
      os << ", ";
    arg.print(os);
    comma = true;
  }
  os << ')';
}

void PrintTo(const AnnotationKind &kind, std::ostream *os) {
  llvm::raw_os_ostream ostream(*os);
  kind.print(ostream);
}

void PrintTo(const Annotation &annotation, std::ostream *os) {
  llvm::raw_os_ostream ostream(*os);
  annotation.print(ostream);
}

} // namespace annotations
} // namespace genpybind
