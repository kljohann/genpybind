#include "genpybind/decl_context_collector.h"

#include <llvm/ADT/STLExtras.h>

using namespace genpybind;

static const char *kLozenge = "◊";

bool genpybind::hasAnnotations(const clang::Decl *decl) {
  return llvm::any_of(decl->specific_attrs<clang::AnnotateAttr>(),
                      [](const auto *attr) {
                        return attr->getAnnotation().startswith(kLozenge);
                      });
}
