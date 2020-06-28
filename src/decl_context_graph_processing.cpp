#include "genpybind/decl_context_graph_processing.h"

#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/DepthFirstIterator.h>
#include <llvm/ADT/PostOrderIterator.h>

#include "genpybind/annotated_decl.h"
#include "genpybind/diagnostics.h"

using namespace genpybind;

namespace {
// Namespaces and unnamed declaration contexts might contain free functions,
// aliases or other declarations that have been marked visible, but which are
// not represented in the declaration context graph.  As these declarations
// will only be exposed if the corresponding parent context node is present,
// these nodes have to be preserved.
class NodesToKeepWhenPruning {
  using NodeRef = const DeclContextNode *;
  const DeclContextGraph *graph;
  const AnnotationStorage &annotations;
  llvm::DenseSet<NodeRef> should_be_preserved;
  using ConstNodeSet = llvm::SmallPtrSetImpl<const DeclContextNode *>;

public:
  NodesToKeepWhenPruning(const DeclContextGraph *graph,
                         const AnnotationStorage &annotations)
      : graph(graph), annotations(annotations) {
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
      return getEffectiveVisibility(decl);

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
    bool default_visibility = getEffectiveVisibility(node->getDecl());
    const auto *context = llvm::cast<clang::DeclContext>(node->getDecl());
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

  bool getEffectiveVisibility(const clang::Decl *decl) const {
    // Anything inside an export decl is visible by default.
    if (llvm::isa<clang::ExportDecl>(decl))
      return true;

    // Else refer to the declaration's effective visibility, which should have
    // been put into place by visibility propagation.  For unnamed declaration
    // contexts, the nearest named parent is used.  This is fine, since unnamed
    // declaration contexts cannot be moved using `expose_here`.
    while (decl != nullptr && !llvm::isa<clang::NamedDecl>(decl))
      decl = llvm::cast_or_null<clang::Decl>(decl->getLexicalDeclContext());

    const auto *named_decl = llvm::dyn_cast_or_null<clang::NamedDecl>(decl);
    if (named_decl == nullptr)
      return false;

    const auto *annotated =
        llvm::dyn_cast_or_null<AnnotatedNamedDecl>(annotations.get(named_decl));
    assert(annotated != nullptr &&
           "annotation should be in place after propagating visibility");
    assert(annotated->visible.hasValue() &&
           "decl should have non-default visibility after propagation");
    return *annotated->visible;
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

DeclContextGraph genpybind::pruneGraph(const DeclContextGraph &graph,
                                       const AnnotationStorage &annotations) {
  // Keep track of unreachable nodes in order to report if any visible
  // declaration context is not part of the pruned graph (because a parent
  // context is hidden).
  llvm::df_iterator_default_set<const DeclContextNode *> reachable;
  NodesToKeepWhenPruning nodes_to_keep(&graph, annotations);

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
