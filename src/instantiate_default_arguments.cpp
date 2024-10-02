// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/instantiate_default_arguments.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Sema/Sema.h>
#include <llvm/Support/Casting.h>

using namespace genpybind;

void InstantiateDefaultArgumentsASTConsumer::HandleTranslationUnit(
    clang::ASTContext &context) {
  TraverseDecl(context.getTranslationUnitDecl());
}

bool InstantiateDefaultArgumentsASTConsumer::VisitParmVarDecl(
    clang::ParmVarDecl *decl) {
  if (decl->hasUnparsedDefaultArg() || !decl->hasUninstantiatedDefaultArg())
    return true;

  auto *function = llvm::dyn_cast<clang::FunctionDecl>(decl->getDeclContext());
  if (function == nullptr || function->isInvalidDecl())
    return true;

  // If the specialization is incomplete, there is no point in continuing.
  if (function->getTemplateSpecializationKind() == clang::TSK_Undeclared)
    return true;

  sema->InstantiateDefaultArgument(clang::SourceLocation(), function, decl);

  return true;
}
