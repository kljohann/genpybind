#include "genpybind/instantiate_alias_targets.h"

#include "genpybind/annotated_decl.h"

#include <clang/Sema/Sema.h>
#include <clang/Sema/SemaDiagnostic.h>

using namespace genpybind;

void InstantiateAliasTargetsASTConsumer::HandleTranslationUnit(
    clang::ASTContext &context) {
  TraverseDecl(context.getTranslationUnitDecl());

  // Indicate to `HandleTagDeclDefinition` that initial parsing of the TU has
  // completed and any tag decls that are completed from now on need to be
  // analyzed for additional annotated type aliases.
  parsing_completed = true;

  while (!pending.empty()) {
    const clang::TypedefNameDecl *decl = pending.back();
    pending.pop_back();

    clang::QualType qual_type = decl->getUnderlyingType();
    if (!qual_type->isIncompleteType() || qual_type->isDependentType())
      continue;

    // Attempt to perform class template instantiation.  May effect
    // additional calls to `HandleTagDeclDefinition`.
    sema->RequireCompleteType(decl->getLocation(), qual_type,
                              clang::diag::err_incomplete_type);
  }
}

void InstantiateAliasTargetsASTConsumer::HandleTagDeclDefinition(
    clang::TagDecl *decl) {
  // Ignore tag decls that are completed during initial parsing of the TU.
  if (!parsing_completed)
    return;

  // TODO: If there is a nested record, will it get its own call to
  // HandleTagDeclDefinition?
  TraverseDecl(decl);
}

bool InstantiateAliasTargetsASTConsumer::VisitTypedefNameDecl(
    const clang::TypedefNameDecl *decl) {
  if (hasAnnotations(decl))
    pending.push_back(decl);
  return true;
}
