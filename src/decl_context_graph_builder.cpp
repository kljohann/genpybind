#include "genpybind/decl_context_graph_builder.h"

#include <clang/Basic/Diagnostic.h>
#include <llvm/ADT/DepthFirstIterator.h>

#include "genpybind/decl_context_collector.h"

using namespace genpybind;

const clang::TagDecl *DeclContextGraphBuilder::addEdgeForExposeHereAlias(
    const clang::TypedefNameDecl *decl) {
  const auto *parent_context = decl->getLexicalDeclContext();
  const auto *parent = llvm::dyn_cast<clang::Decl>(parent_context);
  const clang::TagDecl *target_decl = decl->getUnderlyingType()->getAsTagDecl();
  assert(target_decl != nullptr &&
         "type aliases can only be used with tag type targets");

  target_decl = target_decl->getDefinition();
  auto inserted = moved_previously.try_emplace(target_decl, decl);
  if (!inserted.second) {
    Diagnostics::report(decl, Diagnostics::Kind::AlreadyExposedElsewhereError)
        << target_decl->getNameAsString();
    Diagnostics::report(inserted.first->getSecond(),
                        Diagnostics::Kind::PreviouslyExposedHereNote)
        << target_decl->getNameAsString();
    return nullptr;
  }

  graph.getOrInsertNode(parent)->addChild(graph.getOrInsertNode(target_decl));
  return target_decl;
}

bool DeclContextGraphBuilder::reportExposeHereCycles(
    const ConstNodeSet &reachable_nodes) const {
  bool has_cycles = false;
  // TODO: Change to a sensible/stable iteration order.  Ideally this
  // would correspond to file order.
  for (const auto &pair : graph) {
    const DeclContextNode *node = pair.getSecond().get();
    if (reachable_nodes.count(node))
      continue;
    // While all unreachable nodes are attached to one of the cycles, reporting
    // is only done on the type alias declarations.
    auto it = moved_previously.find(node->getDecl());
    if (it == moved_previously.end())
      continue;
    const clang::TypedefNameDecl *decl = it->getSecond();
    Diagnostics::report(decl, Diagnostics::Kind::ExposeHereCycleError);
    has_cycles = true;
  }
  return has_cycles;
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
    if (const clang::TagDecl *target_decl =
            addEdgeForExposeHereAlias(alias_decl)) {
      annotated->propagateAnnotations(*annotations.getOrInsert(target_decl));
    }
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

bool DeclContextGraphBuilder::propagateVisibility() {
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

  return !reportExposeHereCycles(reachable);
}
