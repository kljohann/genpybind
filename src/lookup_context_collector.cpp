// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/lookup_context_collector.h"

#include "genpybind/annotated_decl.h"
#include "genpybind/diagnostics.h"

#include <clang/AST/Type.h>
#include <clang/Basic/DiagnosticIDs.h>
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
    const clang::NamedDecl *decl) {
  const auto inserted = first_decls.try_emplace(
      llvm::dyn_cast<clang::NamedDecl>(decl->getCanonicalDecl()), decl);
  if (inserted.second) {
    return;
  }
  const clang::NamedDecl *first_decl = inserted.first->second;
  if (!annotations.equal(decl, first_decl)) {
    Diagnostics::report(
        decl, Diagnostics::Kind::AnnotationsNeedToMatchFirstDeclError);
    Diagnostics::report(first_decl, clang::diag::note_declared_at);
  }
}
