// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/decl_context_graph_processing.h"

#include "genpybind/annotated_decl.h"
#include "genpybind/diagnostics.h"
#include "genpybind/sort_decls.h"
#include "genpybind/visible_decls.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DepthFirstIterator.h>
#include <llvm/ADT/GraphTraits.h>
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Support/Casting.h>

#include <cassert>
#include <queue>
#include <vector>

using namespace genpybind;

/// Return the parent node in the graph, if it exists.
///
/// This corresponds to the previous node on the path in depth-first iteration.
static const DeclContextNode *
getParentNode(const llvm::df_iterator<const DeclContextGraph *> &it) {
  assert(it.getPathLength() > 0 && "path should always contain decl itself");
  // Path includes the root node and the declaration itself.
  if (it.getPathLength() < 2) {
    return nullptr;
  }
  const DeclContextNode *parent = it.getPath(it.getPathLength() - 2);
  assert(parent != nullptr);
  return parent;
}

EnclosingScopeMap
genpybind::findEnclosingScopes(const DeclContextGraph &graph,
                               const AnnotationStorage &annotations) {
  EnclosingScopeMap result;

  // Visit the nodes in a depth-first pre-order traversal, s.t. if a node is
  // visited, the enclosing scope is already known for the parent.
  for (auto it = llvm::df_begin(&graph), end_it = llvm::df_end(&graph);
       it != end_it; ++it) {
    const DeclContextNode *parent = getParentNode(it);
    if (parent == nullptr) {
      // The root node has no parent.
      result[it->getDeclContext()] = nullptr;
      continue;
    }

    const clang::DeclContext *parent_context = parent->getDeclContext();

    // Namespaces are transparent unless they define a submodule.
    // Otherwise the parent itself is the first enclosing lookup context.

    auto is_transparent = [&](const clang::Decl *decl) -> bool {
      const auto *named_decl = llvm::dyn_cast<clang::NamedDecl>(decl);
      if (const auto attrs = annotations.get<NamespaceDeclAttrs>(named_decl))
        return !attrs->module;
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
    if (const DeclContextNode *parent = getParentNode(it)) {
      const clang::DeclContext *parent_context = parent->getDeclContext();
      auto it = result.find(parent_context);
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
    const auto *named_decl = llvm::dyn_cast<clang::NamedDecl>(decl);
    if (const auto attrs = annotations.get<NamedDeclAttrs>(named_decl))
      is_visible = attrs->visible.value_or(is_visible);

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
    const DeclContextGraphBuilder::RelocatedDeclsMap &relocated_decls,
    const clang::SourceManager &source_manager) {
  std::vector<const clang::TypedefNameDecl *> cycle_introducing_alias_decls;
  for (const auto &pair : graph) {
    const DeclContextNode *node = pair.getSecond().get();
    const clang::Decl *decl = node->getDecl();
    if (reachable_contexts.count(node->getDeclContext()) != 0)
      continue;
    // While all unreachable nodes are attached to one of the cycles, reporting
    // is only done on the type alias declarations.
    auto it = relocated_decls.find(decl);
    if (it == relocated_decls.end())
      continue;
    cycle_introducing_alias_decls.push_back(it->getSecond());
  }

  llvm::sort(cycle_introducing_alias_decls,
             IsBeforeInTranslationUnit(source_manager));
  for (const auto *alias_decl : cycle_introducing_alias_decls)
    Diagnostics::report(alias_decl, Diagnostics::Kind::ExposeHereCycleError);

  return !cycle_introducing_alias_decls.empty();
}

ConstDeclContextSet genpybind::declContextsWithVisibleNamedDecls(
    clang::Sema &sema, const DeclContextGraph *graph,
    const AnnotationStorage &annotations,
    const EffectiveVisibilityMap &visibilities) {
  ConstDeclContextSet result;

  auto is_hidden_tag_decl =
      [&visibilities](const clang::DeclContext *context) -> bool {
    if (!llvm::isa<clang::TagDecl>(context))
      return false;
    const auto visible = visibilities.find(context);
    assert(visible != visibilities.end() &&
           "visibility should be known for all nodes");
    return !visible->getSecond();
  };

  auto is_parent_of_context_with_visible_named_decls =
      [&result, &is_hidden_tag_decl](const DeclContextNode *node) -> bool {
    return llvm::any_of(*node, [&](const DeclContextNode *child) {
      const clang::DeclContext *context = child->getDeclContext();
      // Hidden tag declarations conceal the entire corresponding sub-tree.
      return result.count(context) != 0 && !is_hidden_tag_decl(context);
    });
  };

  auto contains_visible_named_decls =
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

    auto is_visible_given_default_visibility =
        [&](const clang::NamedDecl *decl) -> bool {
      if (const auto attrs = annotations.get<NamedDeclAttrs>(decl);
          attrs.has_value() && attrs->visible.has_value()) {
        return *attrs->visible;
      }
      // An unannotated declaration in a "visible" context should be preserved.
      return default_visibility;
    };

    std::vector<const clang::NamedDecl *> decls =
        collectVisibleDeclsFromDeclContext(sema, context);

    return llvm::any_of(decls, [&](const clang::NamedDecl *decl) {
      // Nested declaration contexts that are represented in the graph are taken
      // into account above.
      assert(!DeclContextGraph::accepts(decl));

      // Do not visit implicit declarations, as these cannot have been annotated
      // explicitly by the user.
      if (decl->isImplicit())
        return false;

      return is_visible_given_default_visibility(decl);
    });
  };

  // Visit the nodes in a post-order traversal, s.t. if a node is visited,
  // it's already known whether a contained declaration context contains
  // visible named declarations.
  for (const DeclContextNode *node : llvm::post_order(graph)) {
    const clang::DeclContext *context = node->getDeclContext();
    if (is_parent_of_context_with_visible_named_decls(node) ||
        contains_visible_named_decls(node))
      result.insert(context);
  }

  return result;
}

void genpybind::hideNamespacesBasedOnExposeInAnnotation(
    const DeclContextGraph &graph, const AnnotationStorage &annotations,
    ConstDeclContextSet &contexts_with_visible_decls,
    EffectiveVisibilityMap &visibilities, llvm::StringRef module_name) {
  /// Allows to find all nodes dominated by a specific parent node marked
  /// during depth-first traversal.
  ///
  /// `llvm::df_iterator` does not allow arbitrary pre- and post-order actions,
  /// but calls a "completed" action on a custom "visited" set type, when all
  /// children of a node have been processed.  This is used here to clear the
  /// dominator node.
  struct TrackVisitedAndDominator {
    using NodeRef = const ::genpybind::DeclContextNode *;
    llvm::SmallPtrSet<NodeRef, 8> visited;
    NodeRef dominator = nullptr;

    /// Returns whether the current node is dominated by a previously
    /// marked node.
    [[nodiscard]] bool isDominated() const { return dominator != nullptr; }
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
      const auto attrs = annotations.get<NamespaceDeclAttrs>(namespace_decl);
      return attrs.has_value() && !attrs->only_expose_in.empty() &&
             !llvm::any_of(attrs->only_expose_in, [&](llvm::StringRef name) {
               return name == module_name;
             });
    };
    if (!visited.isDominated()) {
      if (!is_namespace_for_different_module())
        continue;
      visited.setDominator(*it);
    }
    const clang::DeclContext *decl_context = it->getDeclContext();
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
            : contexts_with_visible_decls.count(context) != 0;

    if (!should_be_preserved) {
      it.skipChildren(); // increments the iterator
      continue;
    }

    if (const DeclContextNode *parent = getParentNode(it)) {
      auto *new_node = pruned.getOrInsertNode(decl);
      const clang::Decl *parent_decl = parent->getDecl();
      auto *new_parent = pruned.getOrInsertNode(parent_decl);
      new_parent->addChild(new_node);
    }
    ++it;
  }
  return pruned;
}

void genpybind::reportUnreachableVisibleDeclContexts(
    const DeclContextGraph &graph,
    const ConstDeclContextSet &contexts_with_visible_decls,
    const DeclContextGraphBuilder::RelocatedDeclsMap &relocated_decls,
    const clang::SourceManager &source_manager) {
  std::vector<const clang::Decl *> unreachable_decls;
  for (const clang::DeclContext *context : contexts_with_visible_decls) {
    const auto *decl = llvm::dyn_cast<clang::NamedDecl>(context);
    if (decl == nullptr || graph.getNode(decl) != nullptr)
      continue;

    // Emit warning on the `expose_here` alias, if the declaration was moved.
    auto it = relocated_decls.find(decl);
    if (it != relocated_decls.end())
      decl = it->getSecond();

    unreachable_decls.push_back(decl);
  }

  llvm::sort(unreachable_decls, IsBeforeInTranslationUnit(source_manager));
  for (const auto *decl : unreachable_decls)
    Diagnostics::report(decl, Diagnostics::Kind::UnreachableDeclContextWarning)
        << getNameForDisplay(decl);
}

/// Return nodes of graph in topologically sorted order.
///
/// Nodes that are “large” according to `comp` are given precedence.
///
/// If there are cycles, the returned sequence has less elements than the number
/// of nodes in the graph.  Its last entry corresponds to the largest node that
/// participates in any cycle, while the remaining entries correspond to the
/// nodes that do not participate in a circle in their topologically sorted
/// order.  (Note: A cycle consists of at least two nodes, so this encoding
/// is safe.)
template <class GraphType, class FindPredecessors, class Compare>
static std::vector<typename llvm::GraphTraits<GraphType>::NodeRef>
lexicographicalTopologicalSort(const GraphType &graph,
                               FindPredecessors find_predecessors,
                               Compare comp) {
  using NodeRef = typename llvm::GraphTraits<GraphType>::NodeRef;
  using NodeRefs = std::vector<NodeRef>;
  const unsigned graph_size = llvm::GraphTraits<GraphType>::size(graph);
  llvm::DenseMap<NodeRef, int> num_predecessors(graph_size);
  llvm::DenseMap<NodeRef, NodeRefs> successors(graph_size);
  std::priority_queue<NodeRef, NodeRefs, Compare> ready(comp);

  for (NodeRef node : llvm::nodes(graph)) {
    auto add_predecessor = [&](NodeRef other) {
      successors[other].push_back(node);
      ++num_predecessors[node];
    };
    find_predecessors(node, add_predecessor);
    if (num_predecessors[node] == 0)
      ready.push(node);
  }

  NodeRefs result;
  result.reserve(graph_size);

  while (!ready.empty()) {
    NodeRef node = ready.top();
    ready.pop();

    auto node_successors = successors.find(node);
    if (node_successors != successors.end()) {
      for (NodeRef other : node_successors->getSecond()) {
        if (--num_predecessors[other] == 0)
          ready.push(other);
      }
      successors.erase(node_successors);
    }

    result.push_back(node);
  }

  if (!successors.empty()) {
    // There is a cycle.
    for (auto node_in_cycle : successors) {
      ready.push(node_in_cycle.getFirst());
    }
    assert(!ready.empty());
    NodeRef node = ready.top();
    result.push_back(node);
    assert(result.size() != graph_size && "result should not be ambiguous");
  }

  return result;
}

llvm::SmallVector<const clang::DeclContext *, 0>
genpybind::declContextsSortedByDependencies(
    const DeclContextGraph &graph, const EnclosingScopeMap &parents,
    const clang::SourceManager &source_manager,
    const clang::DeclContext **cycle) {
  auto find_predecessors = [&](const DeclContextNode *node,
                               auto add_predecessor) {
    const clang::DeclContext *decl_context = node->getDeclContext();
    // Add parent context as dependency.
    if (const clang::DeclContext *parent = parents.lookup(decl_context)) {
      const DeclContextNode *parent_node =
          graph.getNode(llvm::cast<clang::Decl>(parent));
      assert(parent_node != nullptr && "parent should be in graph");
      add_predecessor(parent_node);
    }
    // Add all exposed (i.e. part of graph) public bases as dependencies.
    if (const auto *decl = llvm::dyn_cast<clang::CXXRecordDecl>(decl_context)) {
      for (const clang::CXXBaseSpecifier &base : decl->bases()) {
        if (base.getAccessSpecifier() != clang::AS_public)
          continue;
        const clang::TagDecl *base_decl = base.getType()->getAsTagDecl();
        if (const DeclContextNode *base_node =
                graph.getNode(base_decl->getDefinition())) {
          assert(base_node != nullptr && "base should be in graph");
          add_predecessor(base_node);
        }
      }
    }
  };

  IsBeforeInTranslationUnit is_before(source_manager);

  auto nodes = lexicographicalTopologicalSort(
      &graph, find_predecessors,
      [&](const DeclContextNode *lhs, const DeclContextNode *rhs) {
        // Note: Larger elements are sorted earlier, thus the arguments are
        // swapped here.
        return is_before(rhs->getDecl(), lhs->getDecl());
      });

  bool cycle_detected = nodes.size() != graph.size();
  if (cycle_detected) {
    assert(!nodes.empty());
    if (cycle != nullptr)
      *cycle = nodes.back()->getDeclContext();
    nodes.clear();
  }
  llvm::SmallVector<const clang::DeclContext *, 0> result;
  result.reserve(graph.size());
  for (const DeclContextNode *node : nodes)
    result.push_back(node->getDeclContext());
  return result;
}
