#include "genpybind/instantiate_default_arguments.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Sema/Sema.h>
#include <llvm/Support/Casting.h>

using namespace genpybind;

void InstantiateDefaultArgumentsASTConsumer::HandleTranslationUnit(
    clang::ASTContext &context) {
  TraverseDecl(context.getTranslationUnitDecl());
}

bool InstantiateDefaultArgumentsASTConsumer::VisitParmVarDecl(
    clang::ParmVarDecl *decl) {
  if (decl->hasUnparsedDefaultArg() || decl->hasUninstantiatedDefaultArg())
    if (auto function =
            llvm::dyn_cast<clang::FunctionDecl>(decl->getDeclContext()))
      sema->CheckCXXDefaultArgExpr(clang::SourceLocation(), function, decl);
  return true;
}
