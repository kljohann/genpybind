// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/decl_context_graph_builder.h"

#include "genpybind/annotated_decl.h"
#include "genpybind/diagnostics.h"
#include "genpybind/lookup_context_collector.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/Diagnostic.h>
#include <llvm/Support/Casting.h>

#include <cassert>
#include <utility>

using namespace genpybind;

static const clang::Decl *
findLookupContextDecl(const clang::DeclContext *decl_context) {
  while (decl_context != nullptr && !decl_context->isLookupContext())
    decl_context = decl_context->getParent();
  return llvm::dyn_cast_or_null<clang::Decl>(decl_context);
}

bool DeclContextGraphBuilder::addEdgeForExposeHereAlias(
    const clang::TypedefNameDecl *decl) {
  const clang::Decl *parent = findLookupContextDecl(decl->getDeclContext());
  const clang::TagDecl *target_decl = aliasTarget(decl);
  assert(target_decl != nullptr);

  auto inserted = relocated_decls.try_emplace(target_decl, decl);
  if (!inserted.second) {
    Diagnostics::report(decl, Diagnostics::Kind::AlreadyExposedElsewhereError)
        << target_decl->getNameAsString();
    Diagnostics::report(inserted.first->getSecond(),
                        Diagnostics::Kind::PreviouslyExposedHereNote)
        << target_decl->getNameAsString();
    return false;
  }

  graph.getOrInsertNode(parent)->addChild(graph.getOrInsertNode(target_decl));
  return true;
}

std::optional<DeclContextGraph> DeclContextGraphBuilder::buildGraph() {
  clang::DiagnosticErrorTrap trap{
      translation_unit->getASTContext().getDiagnostics()};
  LookupContextCollector visitor(annotations);
  visitor.TraverseDecl(translation_unit);

  // Bail out if there were errors during the first traversal of the AST,
  // e.g. due to invalid annotations.
  if (trap.hasErrorOccurred())
    return std::nullopt;

  for (const clang::TypedefNameDecl *alias_decl : visitor.aliases) {
    const auto attrs = annotations.get<TypedefNameDeclAttrs>(alias_decl);
    assert(attrs.has_value());
    if (!attrs->encourage && !attrs->expose_here)
      continue;
    const clang::TagDecl *target_decl = aliasTarget(alias_decl);
    assert(target_decl != nullptr);
    annotations.insert(target_decl);
    if (attrs->encourage)
      annotations.update<NamedDeclAttrs>(
          target_decl, [](auto &target_attrs) { target_attrs.visible = true; });
    if (attrs->expose_here && addEdgeForExposeHereAlias(alias_decl))
      propagateAnnotations(annotations, alias_decl);
  }

  // Bail out if establishing "expose_here" aliases failed, e.g. due to
  // multiple such aliases for one declaration.
  if (trap.hasErrorOccurred())
    return std::nullopt;

  for (const clang::DeclContext *decl_context : visitor.lookup_contexts) {
    const auto *decl = llvm::dyn_cast<clang::Decl>(decl_context);
    // Do not add original parent-child-edge to graph if decl has been moved.
    if (relocated_decls.count(decl) != 0)
      continue;
    // Expose declarations below their semantic parent.
    const clang::Decl *parent = findLookupContextDecl(decl->getDeclContext());
    graph.getOrInsertNode(parent)->addChild(graph.getOrInsertNode(decl));
  }

  return std::move(graph);
}
