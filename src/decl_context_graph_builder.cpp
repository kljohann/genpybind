#include "genpybind/decl_context_graph_builder.h"

#include <clang/Basic/Diagnostic.h>
#include <llvm/ADT/DepthFirstIterator.h>

#include <utility>

#include "genpybind/decl_context_collector.h"

using namespace genpybind;

using annotations::Annotation;
using annotations::AnnotationKind;

auto DeclContextGraphBuilder::aliasTarget(const clang::TypedefNameDecl *decl)
    -> const clang::TagDecl * {
  const clang::TagDecl *target_decl = decl->getUnderlyingType()->getAsTagDecl();
  assert(target_decl != nullptr &&
         "type aliases can only be used with tag type targets");
  return target_decl->getDefinition();
}

bool DeclContextGraphBuilder::addEdgeForExposeHereAlias(
    const clang::TypedefNameDecl *decl) {
  const auto *parent_context = decl->getLexicalDeclContext();
  const auto *parent = llvm::dyn_cast<clang::Decl>(parent_context);
  const clang::TagDecl *target_decl = aliasTarget(decl);

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

llvm::Optional<DeclContextGraph> DeclContextGraphBuilder::buildGraph() {
  clang::DiagnosticErrorTrap trap{
      translation_unit->getASTContext().getDiagnostics()};
  DeclContextCollector visitor(annotations);
  visitor.TraverseDecl(translation_unit);

  // Bail out if there were errors during the first traversal of the AST,
  // e.g. due to invalid annotations.
  if (trap.hasErrorOccurred())
    return llvm::None;

  for (const clang::TypedefNameDecl *alias_decl : visitor.aliases) {
    const auto *annotated = llvm::dyn_cast_or_null<AnnotatedTypedefNameDecl>(
        annotations.get(alias_decl));
    assert(annotated != nullptr &&
           "only aliases with annotations are collected");
    if (!annotated->encourage && !annotated->expose_here)
      continue;
    const clang::TagDecl *target_decl = aliasTarget(alias_decl);
    AnnotatedDecl &annotated_target = *annotations.getOrInsert(target_decl);
    if (annotated->encourage)
      annotated_target.processAnnotation(Annotation(AnnotationKind::Visible));
    if (annotated->expose_here && addEdgeForExposeHereAlias(alias_decl))
      annotated->propagateAnnotations(annotated_target);
  }

  // Bail out if establishing "expose_here" aliases failed, e.g. due to
  // multiple such aliases for one declaration.
  if (trap.hasErrorOccurred())
    return llvm::None;

  for (const clang::DeclContext *decl_context : visitor.decl_contexts) {
    const auto *decl = llvm::dyn_cast<clang::Decl>(decl_context);
    // Do not add original parent-child-edge to graph if decl has been moved.
    if (relocated_decls.count(decl))
      continue;
    const auto *parent =
        llvm::dyn_cast<clang::Decl>(decl->getLexicalDeclContext());
    graph.getOrInsertNode(parent)->addChild(graph.getOrInsertNode(decl));
  }

  return std::move(graph);
}
