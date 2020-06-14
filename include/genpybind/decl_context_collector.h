#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/ADT/DenseSet.h>

namespace genpybind {

bool hasAnnotations(const clang::Decl *decl);

/// Recursive AST visitor that collects all declaration contexts that could
/// possibly contain completely defined `TagDecl`s with external linkage.
/// Also collects annotated `TypedefNameDecl`s, since those could be annotated
/// with `expose_here` annotations that are relevant for the proper nesting of
/// the declaration contexts.

class DeclContextCollector
    : public clang::RecursiveASTVisitor<DeclContextCollector> {
public:
  llvm::DenseSet<const clang::DeclContext *> decl_contexts;
  llvm::DenseSet<const clang::TypedefNameDecl *> aliases;

  bool shouldWalkTypesOfTypeLocs() const { return false; }
  bool shouldVisitTemplateInstantiations() const { return true; }
  bool shouldVisitImplicitCode() const { return false; }

  bool TraverseStmt(clang::Stmt *) {
    // No need to visit statements and expressions.
    return true;
  }

  bool VisitTypedefNameDecl(const clang::TypedefNameDecl *decl) {
    // Only typedefs with explicit annotations are considered.
    if (!hasAnnotations(decl))
      return true;
    aliases.insert(decl);
    return true;
  }

  bool VisitNamespaceDecl(const clang::NamespaceDecl *decl) {
    decl_contexts.insert(decl);
    return true;
  }

  bool VisitTagDecl(const clang::TagDecl *decl) {
    if (!decl->isCompleteDefinition())
      return true;
    decl_contexts.insert(decl);
    return true;
  }

  bool VisitExportDecl(const clang::ExportDecl *decl) {
    decl_contexts.insert(decl);
    return true;
  }

  bool VisitLinkageSpecDecl(const clang::LinkageSpecDecl *decl) {
    decl_contexts.insert(decl);
    return true;
  }
};

}  // namespace genpybind
