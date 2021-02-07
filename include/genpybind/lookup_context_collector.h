#pragma once

#include "genpybind/annotated_decl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include <vector>

namespace clang {
class Stmt;
} // namespace clang

namespace genpybind {

/// Recursive AST visitor that collects all lookup contexts.
///
/// Also collects annotated `TypedefNameDecl`s, since those could be annotated
/// with `expose_here` annotations that are relevant for the proper nesting of
/// the declaration contexts.
///
/// As errors on annotations should be reported in the correct source order and
/// the annotations for aliases and declaration contexts need to be available
/// when building the declaration context graph, annotations for all
/// declarations are already extracted on this first pass through the AST.
///
/// Only non-dependent contexts are considered, as only complete types can be
/// exposed in any case.  As a preparation, `InstantiateAliasTargetsASTConsumer`
/// should have been run, s.t. there is a complete record declaration
/// (`ClassTemplateSpecializationDecl`) for each referenced template
/// instantiation.
class LookupContextCollector
    : public clang::RecursiveASTVisitor<LookupContextCollector> {
  AnnotationStorage &annotations;

public:
  std::vector<const clang::DeclContext *> lookup_contexts;
  std::vector<const clang::TypedefNameDecl *> aliases;

  LookupContextCollector(AnnotationStorage &annotations)
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
    if (!hasAnnotations(decl) || decl->getDeclContext()->isDependentContext())
      return true;
    annotations.getOrInsert(decl);
    return true;
  }

  void warnIfAliasHasQualifiers(const clang::TypedefNameDecl *decl);

  void errorIfAnnotationsDoNotMatchCanonicalDecl(const clang::Decl *decl);

  bool VisitTypedefNameDecl(const clang::TypedefNameDecl *decl) {
    // Only typedefs with explicit annotations are considered.
    if (!hasAnnotations(decl) || decl->getDeclContext()->isDependentContext())
      return true;
    warnIfAliasHasQualifiers(decl);
    aliases.push_back(decl);
    return true;
  }

  bool shouldSkipNamespace(const clang::NamespaceDecl *decl) {
    // Skip implementation namespaces like `std` and `__gnu_cxx`, as these
    // cannot have annotations in any case.
    return decl->isStdNamespace() || decl->getName().startswith("__");
  }

  bool TraverseNamespaceDecl(clang::NamespaceDecl *decl) {
    if (shouldSkipNamespace(decl))
      return true;
    return RecursiveASTVisitor::TraverseNamespaceDecl(decl);
  }

  bool VisitNamespaceDecl(const clang::NamespaceDecl *decl) {
    if (shouldSkipNamespace(decl))
      return true;
    lookup_contexts.push_back(decl);
    errorIfAnnotationsDoNotMatchCanonicalDecl(decl);
    return true;
  }

  bool VisitTagDecl(const clang::TagDecl *decl) {
    if (!decl->isCompleteDefinition() || decl->isDependentType())
      return true;
    lookup_contexts.push_back(decl);
    return true;
  }
};

} // namespace genpybind