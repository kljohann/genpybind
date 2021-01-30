#include "genpybind/decl_context_graph_processing.h"

#include "genpybind/annotated_decl.h"
#include "genpybind/diagnostics.h"
#include "genpybind/visible_decls.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/DepthFirstIterator.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/PointerIntPair.h>
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/Support/Casting.h>

#include <cassert>
#include <memory>

using namespace genpybind;

EnclosingScopeMap
genpybind::findEnclosingScopes(const DeclContextGraph &graph,
                               const AnnotationStorage &annotations) {
  EnclosingScopeMap result;

  // Visit the nodes in a depth-first pre-order traversal, s.t. if a node is
  // visited, the enclosing scope is already known for the parent.
  for (auto it = llvm::df_begin(&graph), end_it = llvm::df_end(&graph);
       it != end_it; ++it) {
    assert(it.getPathLength() > 0 && "path should always contain decl itself");

    // The root node has no parent.
    if (it.getPathLength() < 2) {
      result[it->getDeclContext()] = nullptr;
      continue;
    }

    const DeclContextNode *parent = it.getPath(it.getPathLength() - 2);
    const clang::DeclContext *parent_context = parent->getDeclContext();

    // Namespaces are transparent unless they define a submodule.
    // Otherwise the parent itself is the first enclosing lookup context.

    auto is_transparent = [&](const clang::Decl *decl) -> bool {
      if (const auto *annotated_ns =
              llvm::dyn_cast_or_null<AnnotatedNamespaceDecl>(
                  annotations.get(decl)))
        return !annotated_ns->module;
      return false;
    };

    if (is_transparent(parent->getDecl())) {
      parent_context = result.lookup(parent_context);
      assert(parent_context != nullptr &&
             "parent should have already been processed");
    }

    result[it->getDeclContext()] = parent_context;
  }

  return result;
}

EffectiveVisibilityMap
genpybind::deriveEffectiveVisibility(const DeclContextGraph &graph,
                                     const AnnotationStorage &annotations) {
  EffectiveVisibilityMap result;

  // Visit the nodes in a depth-first pre-order traversal, s.t. if a node is
  // visited, visibility is already assigned to the parent.
  for (auto it = llvm::df_begin(&graph), end_it = llvm::df_end(&graph);
       it != end_it; ++it) {
    const clang::Decl *const decl = it->getDecl();
    const clang::DeclContext *const decl_context = it->getDeclContext();

    // The visibility of the parent context is used as the default visibility.
    // The root node (`TranslationUnitDecl`) is implicitly hidden.
    bool is_visible = false;
    if (it.getPathLength() >= 2) {
      const clang::DeclContext *ancestor =
          it.getPath(it.getPathLength() - 2)->getDeclContext();
      auto it = result.find(ancestor);
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
    if (const auto *annotated =
            llvm::dyn_cast_or_null<AnnotatedNamedDecl>(annotations.get(decl)))
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
    if (reachable_contexts.count(node->getDeclContext()))
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

ConstDeclContextSet genpybind::declContextsWithVisibleNamedDecls(
    clang::Sema &sema, const DeclContextGraph *graph,
    const AnnotationStorage &annotations,
    const EffectiveVisibilityMap &visibilities) {
  ConstDeclContextSet result;

  auto isHiddenTagDecl =
      [&visibilities](const clang::DeclContext *context) -> bool {
    if (!llvm::isa<clang::TagDecl>(context))
      return false;
    const auto visible = visibilities.find(context);
    assert(visible != visibilities.end() &&
           "visibility should be known for all nodes");
    return !visible->getSecond();
  };

  auto isParentOfContextWithVisibleNamedDecls =
      [&result, &isHiddenTagDecl](const DeclContextNode *node) -> bool {
    for (const DeclContextNode *child : *node) {
      const clang::DeclContext *context = child->getDeclContext();
      // Hidden tag declarations conceal the entire corresponding sub-tree.
      if (result.count(context) && !isHiddenTagDecl(context))
        return true;
    }
    return false;
  };

  auto containsVisibleNamedDecls =
      [&](const DeclContextNode *const node) -> bool {
    const clang::DeclContext *const context = node->getDeclContext();

    // Check if there are any visible nested tag declarations.
    // As tag declarations could have been moved here using `expose_here`, the
    // graph is used instead of iterating the declarations stored in `context`.
    // Namespaces do not need to be considered here, as these only serve to
    // provide a default visibility.  Named decls not represented in the graph
    // are handled below.
    for (const DeclContextNode *child : *node) {
      const auto visible = visibilities.find(child->getDeclContext());
      assert(visible != visibilities.end() &&
             "visibility should be known for all nodes");
      if (llvm::isa<clang::TagDecl>(child->getDecl()) && visible->getSecond())
        return true;
    }

    const auto visible = visibilities.find(context);
    assert(visible != visibilities.end() &&
           "visibility should be known for all nodes");
    const bool default_visibility = visible->getSecond();

    auto isVisibleGivenDefaultVisibility =
        [&](const clang::NamedDecl *decl) -> bool {
      const auto *annotated =
          llvm::dyn_cast_or_null<AnnotatedNamedDecl>(annotations.get(decl));
      // An unannotated declaration in a "visible" context should be preserved.
      if (annotated == nullptr || !annotated->visible.hasValue())
        return default_visibility;
      return *annotated->visible;
    };

    std::vector<const clang::NamedDecl *> decls =
        collectVisibleDeclsFromDeclContext(sema, context);
    for (const clang::NamedDecl *decl : decls) {
      // Nested declaration contexts that are represented in the graph are taken
      // into account above.
      assert(!DeclContextGraph::accepts(decl));

      // Do not visit implicit declarations, as these cannot have been annotated
      // explicitly by the user.
      if (decl->isImplicit())
        continue;

      if (isVisibleGivenDefaultVisibility(decl))
        return true;
    }

    return false;
  };

  // Visit the nodes in a post-order traversal, s.t. if a node is visited,
  // it's already known whether a contained declaration context contains
  // visible named declarations.
  for (const DeclContextNode *node : llvm::post_order(graph)) {
    const clang::DeclContext *context = node->getDeclContext();
    if (isParentOfContextWithVisibleNamedDecls(node) ||
        containsVisibleNamedDecls(node))
      result.insert(context);
  }

  return result;
}

void genpybind::hideNamespacesBasedOnExposeInAnnotation(
    const DeclContextGraph &graph, const AnnotationStorage &annotations,
    ConstDeclContextSet &contexts_with_visible_decls,
    EffectiveVisibilityMap &visibilities,
    llvm::StringRef module_name) {
  /// Keep track which nodes were already visited in a depth-first traversal
  /// of the visibility tree and whether the current node is dominated by a
  /// one specific node. (This could be more easily implemented using pre-
  /// and post-order actions.)
  struct TrackVisitedAndDominator {
    using NodeRef = const ::genpybind::DeclContextNode *;
    llvm::SmallPtrSet<NodeRef, 8> visited;
    NodeRef dominator = nullptr;

    bool isDominated() const { return dominator != nullptr; }
    void setDominator(NodeRef node) {
      assert(dominator == nullptr);
      dominator = node;
    }

    auto insert(NodeRef node) -> decltype(visited.insert(node)) {
      return visited.insert(node);
    }

    void completed(NodeRef node) {
      if (node == dominator)
        dominator = nullptr;
    }
  };

  /// Traverse the decl context graph and mark all nodes below a namespace as
  /// hidden, if the namespace belongs to a different module.
  TrackVisitedAndDominator visited;

  for (auto it = llvm::df_ext_begin(&graph, visited),
            end_it = llvm::df_ext_end(&graph, visited);
       it != end_it; ++it) {
    auto is_namespace_for_different_module = [&] {
      const auto *namespace_decl =
          llvm::dyn_cast<clang::NamespaceDecl>(it->getDecl());
      const auto *annotation = llvm::cast_or_null<AnnotatedNamespaceDecl>(
          annotations.get(namespace_decl));
      return annotation != nullptr && !annotation->only_expose_in.empty() &&
             annotation->only_expose_in != module_name;
    };
    if (!visited.isDominated()) {
      if (!is_namespace_for_different_module())
        continue;
      visited.setDominator(*it);
    }
    const clang::DeclContext* decl_context = it->getDeclContext();
    contexts_with_visible_decls.erase(decl_context);
    visibilities[decl_context] = false;
  }
}

DeclContextGraph
genpybind::pruneGraph(const DeclContextGraph &graph,
                      const ConstDeclContextSet &contexts_with_visible_decls,
                      const EffectiveVisibilityMap &visibilities) {
  DeclContextGraph pruned(
      llvm::cast<clang::TranslationUnitDecl>(graph.getRoot()->getDecl()));
  for (auto it = llvm::df_begin(&graph), end_it = llvm::df_end(&graph);
       it != end_it;
       /* incremented below, due to use of `skipChildren` */) {
    const clang::Decl *const decl = it->getDecl();
    const clang::DeclContext *const context = it->getDeclContext();
    const auto visible = visibilities.find(context);
    assert(visible != visibilities.end() &&
           "visibility should be known for all nodes");

    const bool should_be_preserved =
        llvm::isa<clang::TagDecl>(decl)
            ? visible->getSecond()
            : contexts_with_visible_decls.count(context);

    if (!should_be_preserved) {
      it.skipChildren(); // increments the iterator
      continue;
    }

    // Path includes the root node and the declaration itself.
    if (it.getPathLength() < 2) {
      ++it;
      continue;
    }

    auto *node = pruned.getOrInsertNode(decl);
    const clang::Decl *parent_decl =
        it.getPath(it.getPathLength() - 2)->getDecl();
    auto *parent_node = pruned.getOrInsertNode(parent_decl);
    parent_node->addChild(node);

    ++it;
  }
  return pruned;
}

void genpybind::reportUnreachableVisibleDeclContexts(
    const DeclContextGraph &graph,
    const ConstDeclContextSet &contexts_with_visible_decls,
    const DeclContextGraphBuilder::RelocatedDeclsMap &relocated_decls) {
  // TODO: Change to a sensible/stable iteration order.
  for (const clang::DeclContext *context : contexts_with_visible_decls) {
    const auto *decl = llvm::dyn_cast<clang::NamedDecl>(context);
    if (decl == nullptr || graph.getNode(decl) != nullptr)
      continue;

    // Emit warning on the `expose_here` alias, if the declaration was moved.
    auto it = relocated_decls.find(decl);
    if (it != relocated_decls.end())
      decl = it->getSecond();

    Diagnostics::report(decl, Diagnostics::Kind::UnreachableDeclContextWarning)
        << getNameForDisplay(decl);
  }
}

llvm::SmallVector<const clang::DeclContext *, 0>
genpybind::declContextsSortedByDependencies(const DeclContextGraph &graph,
                                            const EnclosingScopeMap &parents,
                                            const clang::DeclContext **cycle) {
  llvm::SmallVector<const clang::DeclContext *, 0> result;
  llvm::DenseSet<const clang::DeclContext *> started, finished;
  // Boolean encodes whether all dependencies have been visited.
  using WorklistItem =
      llvm::PointerIntPair<const clang::DeclContext *, 1, bool>;
  llvm::SmallVector<WorklistItem, 0> worklist;
  result.reserve(graph.size());
  started.reserve(graph.size());
  finished.reserve(graph.size());
  worklist.reserve(2 * graph.size());
  for (const DeclContextNode *node : llvm::depth_first(&graph)) {
    worklist.push_back({node->getDeclContext(), false});
  }

  while (!worklist.empty()) {
    const WorklistItem item = worklist.back();
    worklist.pop_back();
    const clang::DeclContext *decl_context = item.getPointer();
    bool after_dependencies = item.getInt();

    if (finished.count(decl_context))
      continue;

    if (after_dependencies) {
      finished.insert(decl_context);
      result.push_back(decl_context);
      continue;
    }

    bool cycle_detected = !started.insert(decl_context).second;
    if (cycle_detected) {
      if (cycle != nullptr)
        *cycle = decl_context;
      result.clear();
      break;
    }

    // Re-visit this node after all its dependencies.
    worklist.push_back({decl_context, true});
    // Add parent context as dependency.
    if (const clang::DeclContext *parent = parents.lookup(decl_context)) {
      assert(graph.getNode(llvm::cast<clang::Decl>(parent)) != nullptr &&
             "parent should be in graph");
      worklist.push_back({parent, false});
    }
    // Add all exposed (i.e. part of graph) public bases as dependencies.
    if (const auto *decl = llvm::dyn_cast<clang::CXXRecordDecl>(decl_context)) {
      for (const clang::CXXBaseSpecifier &base : decl->bases()) {
        if (base.getAccessSpecifier() != clang::AS_public)
          continue;
        const clang::TagDecl *base_decl = base.getType()->getAsTagDecl();
        if (const DeclContextNode *base_node =
                graph.getNode(base_decl->getDefinition())) {
          worklist.push_back({base_node->getDeclContext(), false});
        }
      }
    }
  }

  return result;
}
