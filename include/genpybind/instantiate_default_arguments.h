// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Sema/SemaConsumer.h>

namespace clang {
class ASTContext;
class Sema;
class Stmt;
class ParmVarDecl;
} // namespace clang

namespace genpybind {

/// Force default argument instantiation, s.t. the corresponding `Expr` node is
/// present in the AST.  (This is normally done lazily when gathering arguments
/// at the call site.)
class InstantiateDefaultArgumentsASTConsumer
    : public clang::SemaConsumer,
      public clang::RecursiveASTVisitor<
          InstantiateDefaultArgumentsASTConsumer> {
  clang::Sema *sema = nullptr;

public:
  void InitializeSema(clang::Sema &sema_) override { sema = &sema_; }
  void ForgetSema() override { sema = nullptr; }

  void HandleTranslationUnit(clang::ASTContext &context) override;

  bool shouldWalkTypesOfTypeLocs() const { return false; }
  bool shouldVisitTemplateInstantiations() const { return true; }
  bool shouldVisitImplicitCode() const { return false; }

  bool TraverseStmt(clang::Stmt *) {
    // Do not visit statements and expressions.
    return true;
  }

  bool VisitParmVarDecl(clang::ParmVarDecl *decl);
};

} // namespace genpybind
