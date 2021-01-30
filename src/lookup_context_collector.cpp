#include "genpybind/lookup_context_collector.h"

#include "genpybind/annotated_decl.h"
#include "genpybind/diagnostics.h"

#include <clang/AST/Type.h>

using namespace genpybind;

void LookupContextCollector::warnIfAliasHasQualifiers(
    const clang::TypedefNameDecl *decl) {
  clang::QualType qual_type = decl->getUnderlyingType();
  if (qual_type.hasQualifiers())
    Diagnostics::report(decl,
                        Diagnostics::Kind::IgnoringQualifiersOnAliasWarning);
}

void LookupContextCollector::errorIfAnnotationsDoNotMatchCanonicalDecl(
    const clang::Decl *decl) {
  if (decl->isCanonicalDecl())
    return;
  const auto *canonical_decl = decl->getCanonicalDecl();
  if (getAnnotationStrings(decl) == getAnnotationStrings(canonical_decl))
    return;
  Diagnostics::report(
      decl, Diagnostics::Kind::AnnotationsNeedToMatchCanonicalDeclError)
      << static_cast<unsigned>(llvm::isa<clang::NamespaceDecl>(decl));
  Diagnostics::report(canonical_decl, clang::diag::note_declared_at);
}
