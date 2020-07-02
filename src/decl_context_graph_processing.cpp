#include "genpybind/decl_context_graph_processing.h"

#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/DepthFirstIterator.h>
#include <llvm/ADT/PostOrderIterator.h>

#include "genpybind/annotated_decl.h"
#include "genpybind/diagnostics.h"

using namespace genpybind;

EffectiveVisibilityMap
genpybind::deriveEffectiveVisibility(const DeclContextGraph &graph,
                                     const AnnotationStorage &annotations) {
  EffectiveVisibilityMap result;

  // Visit the nodes in a depth-first pre-order traversal, s.t. if a node is
  // visited, visibility is already assigned to the parent.
  for (auto it = llvm::df_begin(&graph), end_it = llvm::df_end(&graph);
       it != end_it; ++it) {
    const clang::Decl *const decl = it->getDecl();
    const auto *const decl_context = llvm::cast<clang::DeclContext>(decl);

    // Export declarations (and nested declarations) are visible by default.
    if (llvm::isa<clang::ExportDecl>(decl)) {
      result[decl_context] = true;
      continue;
    }

    // The visibility of the parent context is used as the default visibility.
    // The root node (`TranslationUnitDecl`) is implicitly hidden.
    bool is_visible = false;
    if (it.getPathLength() >= 2) {
      const clang::Decl *ancestor =
          it.getPath(it.getPathLength() - 2)->getDecl();
      auto it = result.find(llvm::cast<clang::DeclContext>(ancestor));
      assert(it != result.end() &&
             "visibility of parent should have already been updated");
      is_visible = it->getSecond();
    }

    // Declarations with protected and private access specifiers are hidden.
    switch (decl->getAccess()) {
    case clang::AS_protected:
    case clang::AS_private:
      is_visible = false;
      break;
    case clang::AS_public:
    case clang::AS_none:
      // Do nothing.
      break;
    }

    // Retrieve value from annotations, if specified explicitly.
    if (const auto *named_decl = llvm::dyn_cast<clang::NamedDecl>(decl))
      if (const auto *annotated = llvm::dyn_cast_or_null<AnnotatedNamedDecl>(
              annotations.get(named_decl)))
        is_visible = annotated->visible.getValueOr(is_visible);

    result[decl_context] = is_visible;
  }

  return result;
}

ConstDeclContextSet
genpybind::reachableDeclContexts(const EffectiveVisibilityMap &visibilities) {
  ConstDeclContextSet result;
  for (const auto &pair : visibilities) {
    result.insert(pair.getFirst());
  }
  return result;
}

bool genpybind::reportExposeHereCycles(
    const DeclContextGraph &graph,
    const ConstDeclContextSet &reachable_contexts,
    const DeclContextGraphBuilder::RelocatedDeclsMap &relocated_decls) {
  bool has_cycles = false;
  // TODO: Change to a sensible/stable iteration order.  Ideally this
  // would correspond to file order.
  for (const auto &pair : graph) {
    const DeclContextNode *node = pair.getSecond().get();
    const clang::Decl *decl = node->getDecl();
    if (reachable_contexts.count(llvm::cast<clang::DeclContext>(decl)))
      continue;
    // While all unreachable nodes are attached to one of the cycles, reporting
    // is only done on the type alias declarations.
    auto it = relocated_decls.find(decl);
    if (it == relocated_decls.end())
      continue;
    const clang::TypedefNameDecl *alias_decl = it->getSecond();
    Diagnostics::report(alias_decl, Diagnostics::Kind::ExposeHereCycleError);
    has_cycles = true;
  }
  return has_cycles;
}

namespace {
class NodesToKeepWhenPruning {
  using NodeRef = const DeclContextNode *;
  const DeclContextGraph *graph;
  const AnnotationStorage &annotations;
  const EffectiveVisibilityMap &visibilities;
  llvm::DenseSet<NodeRef> should_be_preserved;
  using ConstNodeSet = llvm::SmallPtrSetImpl<const DeclContextNode *>;

public:
  NodesToKeepWhenPruning(const DeclContextGraph *graph,
                         const AnnotationStorage &annotations,
                         const EffectiveVisibilityMap &visibilities)
      : graph(graph), annotations(annotations), visibilities(visibilities) {
    for (const DeclContextNode *node : llvm::post_order(graph)) {
      if (shouldPreserve(node))
        should_be_preserved.insert(node);
    }
  }

  bool contains(NodeRef node) const { return should_be_preserved.count(node); }

  void
  reportUnreachableVisibleNodes(const ConstNodeSet &reachable_nodes) const {
    // TODO: Change to a sensible/stable iteration order.
    for (const auto &pair : *graph) {
      const DeclContextNode *node = pair.getSecond().get();
      if (reachable_nodes.count(node))
        continue;

      const clang::NamedDecl *const decl =
          llvm::dyn_cast<clang::NamedDecl>(node->getDecl());

      if (decl == nullptr)
        continue;

      if (isParentOfPreservedDeclarationContext(node) ||
          containsVisibleDeclarations(node))
        Diagnostics::report(decl,
                            Diagnostics::Kind::UnreachableDeclContextWarning);
    }
  }

private:
  bool shouldPreserve(NodeRef node) const {
    const clang::Decl *decl = node->getDecl();
    if (llvm::isa<clang::TagDecl>(decl))
      return getEffectiveVisibility(llvm::cast<clang::DeclContext>(decl));

    return isParentOfPreservedDeclarationContext(node) ||
           containsVisibleDeclarations(node);
  }

  bool isParentOfPreservedDeclarationContext(NodeRef node) const {
    for (const DeclContextNode *child : *node) {
      if (contains(child))
        return true;
    }
    return false;
  }

  bool containsVisibleDeclarations(NodeRef node) const {
    const auto *context = llvm::cast<clang::DeclContext>(node->getDecl());
    const bool default_visibility = getEffectiveVisibility(context);
    for (clang::DeclContext::specific_decl_iterator<clang::NamedDecl>
             it(context->decls_begin()),
         end_it(context->decls_end());
         it != end_it; ++it) {
      // Skip declaration contexts represented in the graph, as those are
      // handled above.
      if (graph->getNode(*it) != nullptr)
        continue;
      // Do not visit implicit declarations, as these cannot have been annotated
      // explicitly by the user.  This avoids false positives on the implicit
      // `CXXRecordDecl` nested inside C++ records to represent the
      // injected-class-name.
      if (it->isImplicit())
        continue;
      if (isVisibleGivenDefaultVisibility(*it, default_visibility))
        return true;
    }
    return false;
  }

  bool getEffectiveVisibility(const clang::DeclContext *context) const {
    const auto it = visibilities.find(context);
    assert(it != visibilities.end() &&
           "visibility should be known for all nodes");
    return it->getSecond();
  }

  bool isVisibleGivenDefaultVisibility(const clang::NamedDecl *decl,
                                       bool default_visibility) const {
    const auto *annotated =
        llvm::dyn_cast_or_null<AnnotatedNamedDecl>(annotations.get(decl));
    // An unannotated declaration in a "visible" context should be preserved.
    if (annotated == nullptr || !annotated->visible.hasValue())
      return default_visibility;
    return *annotated->visible;
  }
};
} // namespace

DeclContextGraph
genpybind::pruneGraph(const DeclContextGraph &graph,
                      const AnnotationStorage &annotations,
                      const EffectiveVisibilityMap &visibilities) {
  // Keep track of unreachable nodes in order to report if any visible
  // declaration context is not part of the pruned graph (because a parent
  // context is hidden).
  llvm::df_iterator_default_set<const DeclContextNode *> reachable;
  NodesToKeepWhenPruning nodes_to_keep(&graph, annotations, visibilities);

  DeclContextGraph pruned(
      llvm::cast<clang::TranslationUnitDecl>(graph.getRoot()->getDecl()));
  for (auto it = llvm::df_ext_begin(&graph, reachable),
            end_it = llvm::df_ext_end(&graph, reachable);
       it != end_it;
       /* incremented below, due to use of `skipChildren` */) {
    if (!nodes_to_keep.contains(*it)) {
      it.skipChildren(); // increments the iterator
      continue;
    }

    // Path includes the root node and the declaration itself.
    if (it.getPathLength() < 2) {
      ++it;
      continue;
    }

    auto *node = pruned.getOrInsertNode(it->getDecl());
    const clang::Decl *parent_decl =
        it.getPath(it.getPathLength() - 2)->getDecl();
    auto *parent_node = pruned.getOrInsertNode(parent_decl);
    parent_node->addChild(node);

    ++it;
  }
  nodes_to_keep.reportUnreachableVisibleNodes(reachable);
  return pruned;
}
