#pragma once

#include "genpybind/annotated_decl.h"
#include "genpybind/diagnostics.h"

#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include <vector>

namespace genpybind {

/// Recursive AST visitor that collects all declaration contexts that could
/// possibly contain completely defined `TagDecl`s with external linkage.
/// Also collects annotated `TypedefNameDecl`s, since those could be annotated
/// with `expose_here` annotations that are relevant for the proper nesting of
/// the declaration contexts.
///
/// As errors on annotations should be reported in the correct source order and
/// the annotations for aliases and declaration contexts need to be available
/// when building the declaration context graph, annotations for all
/// declarations are already extracted on this first pass through the AST.
class DeclContextCollector
    : public clang::RecursiveASTVisitor<DeclContextCollector> {
  AnnotationStorage& annotations;

public:
  std::vector<const clang::DeclContext *> decl_contexts;
  std::vector<const clang::TypedefNameDecl *> aliases;

  DeclContextCollector(AnnotationStorage& annotations)
      : annotations(annotations) {}

  bool shouldWalkTypesOfTypeLocs() const { return false; }
  bool shouldVisitTemplateInstantiations() const { return true; }
  bool shouldVisitImplicitCode() const { return false; }

  bool TraverseStmt(clang::Stmt *) {
    // No need to visit statements and expressions.
    return true;
  }

  bool VisitNamedDecl(const clang::NamedDecl *decl) {
    // Collect all annotated declarations, such that annotation errors are
    // reported in the correct order.
    if (!hasAnnotations(decl))
      return true;
    annotations.getOrInsert(decl);
    return true;
  }

  void warnIfAliasHasQualifiers(const clang::TypedefNameDecl *decl);

  bool VisitTypedefNameDecl(const clang::TypedefNameDecl *decl) {
    // Only typedefs with explicit annotations are considered.
    if (!hasAnnotations(decl))
      return true;
    warnIfAliasHasQualifiers(decl);
    aliases.push_back(decl);
    return true;
  }

  bool VisitNamespaceDecl(const clang::NamespaceDecl *decl) {
    decl_contexts.push_back(decl);
    return true;
  }

  bool VisitTagDecl(const clang::TagDecl *decl) {
    if (!decl->isCompleteDefinition())
      return true;
    decl_contexts.push_back(decl);
    return true;
  }

  bool VisitExportDecl(const clang::ExportDecl *decl) {
    decl_contexts.push_back(decl);
    return true;
  }

  bool VisitLinkageSpecDecl(const clang::LinkageSpecDecl *decl) {
    decl_contexts.push_back(decl);
    return true;
  }
};

}  // namespace genpybind
