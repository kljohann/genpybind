// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Sema/SemaConsumer.h>
#include <llvm/ADT/DenseSet.h>

#include <queue>

namespace clang {
class ASTContext;
class ClassTemplateSpecializationDecl;
class Sema;
class Stmt;
class TypedefNameDecl;
} // namespace clang

namespace genpybind {

/// Recursively visits all class template instantiations that are either
/// annotated themselves or that are the underlying type of an annotated
/// `TypedefNameDecl` and ensures that they are defined, by instantiating
/// the template itself and its members.
/// As a consequence, each instantiation is represented in the AST by a
/// dedicated `CXXRecordDecl`.
class InstantiateAnnotatedTemplatesASTConsumer
    : public clang::SemaConsumer,
      public clang::RecursiveASTVisitor<
          InstantiateAnnotatedTemplatesASTConsumer> {
  clang::Sema *sema = nullptr;
  std::queue<clang::ClassTemplateSpecializationDecl *> pending;
  llvm::DenseSet<const clang::ClassTemplateSpecializationDecl *> done;

  void instantiateSpecialization(
      clang::ClassTemplateSpecializationDecl *specialization);

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

  bool VisitTypedefNameDecl(const clang::TypedefNameDecl *decl);
  bool VisitClassTemplateSpecializationDecl(
      clang::ClassTemplateSpecializationDecl *decl);
};

} // namespace genpybind
