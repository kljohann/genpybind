#include "genpybind/annotations/annotation.h"

#include <llvm/Support/raw_os_ostream.h>

using namespace genpybind::annotations;

llvm::StringRef genpybind::annotations::toString(AnnotationKind kind) {
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

std::string genpybind::annotations::toString(const Annotation &annotation) {
  std::string result;
  llvm::raw_string_ostream stream(result);
  annotation.print(stream);
  return stream.str();
}

void genpybind::annotations::PrintTo(const AnnotationKind &kind,
                                     std::ostream *os) {
  llvm::raw_os_ostream ostream(*os);
  kind.print(ostream);
}

void genpybind::annotations::PrintTo(const Annotation &annotation,
                                     std::ostream *os) {
  llvm::raw_os_ostream ostream(*os);
  annotation.print(ostream);
}
