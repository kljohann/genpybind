#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Sema/SemaConsumer.h>

#include <vector>

namespace genpybind {

/// Recursively visits all annotated `TypedefNameDecl`s that are not in a
/// dependent context and ensures that their underlying type is complete.
/// In particular, any referenced templates will be instantiated, s.t. each
/// instantiation is represented in the AST by a dedicated `CXXRecordDecl`.
///
/// As soon as the AST for entire translation unit has been parsed
/// (`HandleTranslationUnit`), annotated type aliases from non-dependent
/// contexts are collected using a recursive visitor.
/// Those are then presented to `Sema`, in order to complete any referenced
/// record, which will in turn lead to a call to `HandleTagDeclDefinition`,
/// where it can then be analyzed for further nested annotated type aliases.
/// Any earlier calls to that function are ignored.
class InstantiateAliasTargetsASTConsumer
    : public clang::SemaConsumer,
      public clang::RecursiveASTVisitor<InstantiateAliasTargetsASTConsumer> {
  clang::Sema *sema = nullptr;
  bool parsing_completed = false;
  std::vector<const clang::TypedefNameDecl*> pending;

public:
  void InitializeSema(clang::Sema &sema_) override { sema = &sema_; }
  void ForgetSema() override { sema = nullptr; }

  void HandleTranslationUnit(clang::ASTContext &context) override;
  void HandleTagDeclDefinition(clang::TagDecl *decl) override;

  bool shouldWalkTypesOfTypeLocs() const { return false; }
  bool shouldVisitTemplateInstantiations() const { return true; }
  bool shouldVisitImplicitCode() const { return false; }

  bool TraverseStmt(clang::Stmt *) {
    // Do not visit statements and expressions.
    return true;
  }

  bool VisitTypedefNameDecl(const clang::TypedefNameDecl *decl);
};

} // namespace genpybind
