#include "genpybind/decl_context_graph_builder.h"

#include <clang/Basic/Diagnostic.h>
#include <llvm/ADT/DepthFirstIterator.h>

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
    return false;

  for (const clang::DeclContext *decl_context : visitor.decl_contexts) {
    const auto *decl = llvm::dyn_cast<clang::Decl>(decl_context);
    // Do not add original parent-child-edge to graph if decl has been moved.
    if (relocated_decls.count(decl))
      continue;
    const auto *parent =
        llvm::dyn_cast<clang::Decl>(decl->getLexicalDeclContext());
    graph.getOrInsertNode(parent)->addChild(graph.getOrInsertNode(decl));
  }

  return true;
}

void DeclContextGraphBuilder::propagateVisibility() {
  // As a node can only have one parent, `expose_here` cycles are necessarily
  // detached from the root node.  Track the reachable nodes during traversal
  // of the tree in order to find and report those.
  llvm::df_iterator_default_set<const DeclContextNode *> reachable;

  // Visit the nodes in a depth-first pre-order traversal.
  for (auto it = llvm::df_ext_begin(&graph, reachable),
            end_it = llvm::df_ext_end(&graph, reachable);
       it != end_it; ++it) {

    // Visibility annotations can only be attached to named parents.
    const auto *decl = llvm::dyn_cast<clang::NamedDecl>(it->getDecl());
    if (decl == nullptr || it.getPathLength() < 2)
      continue;

    // Any decl at global scope is hidden by default.
    bool default_visibility = false;
    // Find closest ancestor that has a valid visibility.  This needs to skip
    // over intermediate levels which cannot have annotations
    // (e.g. `LinkageSpecDecl`s).
    // Start at -2, as the decl itself is already on the path.
    assert(it.getPathLength() >= 2 &&
           "path should contain at least root node and decl itself");
    for (unsigned index = it.getPathLength() - 2; index > 0; --index) {
      const clang::Decl *parent_decl = it.getPath(index)->getDecl();

      // Anything inside an export decl is visible by default.
      if (llvm::isa<clang::ExportDecl>(parent_decl)) {
        default_visibility = true;
        break;
      }
      // For named parent decls, retrieve their visibility.
      const auto *named_parent = llvm::dyn_cast<clang::NamedDecl>(parent_decl);
      if (named_parent == nullptr)
        continue;

      if (const auto *annotated = llvm::dyn_cast_or_null<AnnotatedNamedDecl>(
              annotations.get(named_parent))) {
        assert(annotated != nullptr && annotated->visible.hasValue() &&
               "visibility of parent should have already been updated");
        default_visibility = annotated->visible.getValue();
        break;
      }
    }

    auto *annotated =
        llvm::cast<AnnotatedNamedDecl>(annotations.getOrInsert(decl));
    annotated->visible = annotated->visible.getValueOr(default_visibility);
  }
}
