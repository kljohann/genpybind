#include "genpybind/lookup_context_collector.h"

#include "genpybind/annotated_decl.h"
#include "genpybind/annotations/literal_value.h"
#include "genpybind/diagnostics.h"

#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Casting.h>

using namespace genpybind;

void LookupContextCollector::warnIfAliasHasQualifiers(
    const clang::TypedefNameDecl *decl) {
  clang::QualType qual_type = decl->getUnderlyingType();
  if (qual_type.hasQualifiers())
    Diagnostics::report(decl,
                        Diagnostics::Kind::IgnoringQualifiersOnAliasWarning);
}

void LookupContextCollector::errorIfAnnotationsDoNotMatchFirstDecl(
    const clang::Decl *decl) {
  const auto inserted = first_decls.try_emplace(decl->getCanonicalDecl(), decl);
  if (inserted.second) {
    return;
  }
  const clang::Decl *first_decl = inserted.first->second;
  const AnnotatedDecl *annotation = annotations.get(decl);
  const AnnotatedDecl *first_annotation = annotations.get(first_decl);
  if ((annotation == nullptr && first_annotation == nullptr) ||
      (annotation != nullptr && annotation->equals(first_annotation)))
    return;
  Diagnostics::report(decl,
                      Diagnostics::Kind::AnnotationsNeedToMatchFirstDeclError);
  Diagnostics::report(first_decl, clang::diag::note_declared_at);
}
