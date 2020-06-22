#include "genpybind/decl_context_graph_builder.h"

#include <clang/Basic/Diagnostic.h>

#include "genpybind/decl_context_collector.h"

using namespace genpybind;

bool DeclContextGraphBuilder::addEdgeForExposeHereAlias(
    const clang::TypedefNameDecl *decl) {
  const auto *parent_context = decl->getLexicalDeclContext();
  const auto *parent = llvm::dyn_cast<clang::Decl>(parent_context);
  const clang::TagDecl *target_decl = decl->getUnderlyingType()->getAsTagDecl();

  // Report error when 'expose_here' is used with non-tag type.
  if (target_decl == nullptr) {
    Diagnostics::report(decl,
                        Diagnostics::Kind::UnsupportedExposeHereTargetError);
    return false;
  }

  target_decl = target_decl->getDefinition();
  auto inserted = moved_previously.try_emplace(target_decl, decl);
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

bool DeclContextGraphBuilder::buildGraph() {
  clang::DiagnosticErrorTrap trap{
      translation_unit->getASTContext().getDiagnostics()};
  DeclContextCollector visitor(annotations);
  visitor.TraverseDecl(translation_unit);

  // Bail out if there were errors during the first traversal of the AST,
  // e.g. due to invalid annotations.
  if (trap.hasErrorOccurred())
    return false;

  for (const clang::TypedefNameDecl *alias_decl : visitor.aliases) {
    const auto *annotated = llvm::dyn_cast_or_null<AnnotatedTypedefNameDecl>(
        annotations.get(alias_decl));
    assert(annotated != nullptr &&
           "only aliases with annotations are collected");
    if (annotated == nullptr || !annotated->expose_here)
      continue;
    addEdgeForExposeHereAlias(alias_decl);
  }

  // Bail out if establishing "expose_here" aliases failed, e.g. due to
  // multiple such aliases for one declaration.
  if (trap.hasErrorOccurred())
    return false;

  for (const clang::DeclContext *decl_context : visitor.decl_contexts) {
    const auto *decl = llvm::dyn_cast<clang::Decl>(decl_context);
    // Do not add original parent-child-edge to graph if decl has been moved.
    if (moved_previously.count(decl))
      continue;
    const auto *parent =
        llvm::dyn_cast<clang::Decl>(decl->getLexicalDeclContext());
    graph.getOrInsertNode(parent)->addChild(graph.getOrInsertNode(decl));
  }

  return true;
}
