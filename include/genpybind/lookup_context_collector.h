// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "genpybind/annotated_decl.h"

#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

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
  llvm::DenseMap<const clang::NamedDecl *, const clang::NamedDecl *>
      first_decls;

public:
  std::vector<const clang::DeclContext *> lookup_contexts;
  std::vector<const clang::TypedefNameDecl *> aliases;

  LookupContextCollector(AnnotationStorage &annotations)
      : annotations(annotations) {}

  static bool shouldWalkTypesOfTypeLocs() { return false; }
  static bool shouldVisitTemplateInstantiations() { return true; }
  static bool shouldVisitImplicitCode() { return false; }

  static bool TraverseStmt(clang::Stmt *) {
    // No need to visit statements and expressions.
    return true;
  }

  static bool shouldSkip(const clang::Decl *decl) {
    return shouldSkip(llvm::dyn_cast<clang::TagDecl>(decl)) ||
           shouldSkip(llvm::dyn_cast<clang::NamespaceDecl>(decl));
  }

  static bool shouldSkip(const clang::TagDecl *decl) {
    return decl != nullptr &&
           (!decl->isCompleteDefinition() || decl->isDependentType());
  }

  static bool shouldSkip(const clang::NamespaceDecl *decl) {
    // Skip implementation namespaces like `std` and `__gnu_cxx`, as these
    // cannot have annotations in any case.
    return decl != nullptr &&
           (decl->isStdNamespace() || decl->getName().starts_with("__"));
  }

  bool VisitNamedDecl(const clang::NamedDecl *decl) {
    // Collect all annotated declarations, such that annotation errors are
    // reported in the correct order.
    if (shouldSkip(llvm::dyn_cast<clang::TagDecl>(decl)) ||
        decl->getDeclContext()->isDependentContext() || !hasAnnotations(decl))
      return true;
    annotations.insert(decl);
    return true;
  }

  static void warnIfAliasHasQualifiers(const clang::TypedefNameDecl *decl);

  void errorIfAnnotationsDoNotMatchFirstDecl(const clang::NamedDecl *decl);

  bool VisitTypedefNameDecl(const clang::TypedefNameDecl *decl) {
    // Only typedefs with explicit annotations are considered.
    if (!hasAnnotations(decl) || decl->getDeclContext()->isDependentContext())
      return true;
    warnIfAliasHasQualifiers(decl);
    aliases.push_back(decl);
    return true;
  }

  bool TraverseNamespaceDecl(clang::NamespaceDecl *decl) {
    if (shouldSkip(decl))
      return true;
    auto before_traversing = annotations.size();
    bool result = RecursiveASTVisitor::TraverseNamespaceDecl(decl);
    bool saw_annotated_decls = annotations.size() != before_traversing;
    if (saw_annotated_decls) {
      // Only check annotations for namespaces that either are annotated
      // themselves or that contain annotated decls.  Other namespaces are
      // pruned in any case and enforcing them to have annotations would only
      // annoy the user.
      errorIfAnnotationsDoNotMatchFirstDecl(decl);
    }
    return result;
  }

  bool VisitNamespaceDecl(const clang::NamespaceDecl *decl) {
    if (shouldSkip(decl))
      return true;
    lookup_contexts.push_back(decl);
    return true;
  }

  bool VisitTagDecl(const clang::TagDecl *decl) {
    if (shouldSkip(decl))
      return true;
    lookup_contexts.push_back(decl);
    return true;
  }
};

} // namespace genpybind
