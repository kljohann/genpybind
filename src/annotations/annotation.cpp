#include "genpybind/annotations/annotation.h"

namespace genpybind {
namespace annotations {

llvm::StringRef AnnotationKind::toString() const {
  switch (kind) {
#define ANNOTATION_KIND(Enum, Spelling)                                        \
  case Kind::Enum:                                                             \
    return #Spelling;
#include "genpybind/annotations/annotations.def"
  }
  llvm_unreachable("Unknown annotation kind.");
}

} // namespace annotations
} // namespace genpybind
