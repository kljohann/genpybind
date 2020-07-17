#include "genpybind/decl_context_graph_processing.h"

#include "genpybind/annotated_decl.h"
#include "genpybind/diagnostics.h"

#include <clang/AST/Decl.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/ADT/DepthFirstIterator.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/Support/Casting.h>

#include <cassert>
#include <memory>

using namespace genpybind;

EnclosingNamedDeclMap genpybind::findEnclosingScopeIntroducingAncestors(
    const DeclContextGraph &graph, const AnnotationStorage &annotations) {
  EnclosingNamedDeclMap result;

  // Visit the nodes in a depth-first pre-order traversal; any other traversal
  // would also do, but `DepthFirstIterator` allows convenient access to
  // the traversed path and thus the node's ancestors.
  for (auto it = llvm::df_begin(&graph), end_it = llvm::df_end(&graph);
       it != end_it; ++it) {
    const clang::NamedDecl *named_ancestor = nullptr;
    assert(it.getPathLength() > 0 && "path should always contain decl itself");
    for (unsigned index = it.getPathLength() - 1; index > 0; --index) {
      const clang::Decl *ancestor = it.getPath(index - 1)->getDecl();

      // Namespaces are transparent unless they define a submodule.
      if (const auto *annotated_ns =
              llvm::dyn_cast_or_null<AnnotatedNamespaceDecl>(
                  annotations.get(ancestor)))
        if (!annotated_ns->module)
          continue;

      // Otherwise use the first enclosing named ancestor.
      named_ancestor = llvm::dyn_cast<clang::NamedDecl>(ancestor);
      if (named_ancestor != nullptr)
        break;
    }
    result[llvm::cast<clang::DeclContext>(it->getDecl())] = named_ancestor;
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

    // Export declarations (and nested declarations) are visible by default.
    if (llvm::isa<clang::ExportDecl>(decl)) {
      result[decl_context] = true;
      continue;
    }

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
    const DeclContextGraph *graph, const AnnotationStorage &annotations,
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
      [&annotations, &visibilities](const DeclContextNode *const node) -> bool {
    const clang::DeclContext *const context = node->getDeclContext();

    // Check if there are any visible nested tag declarations.  Namespaces and
    // unnamed nested declaration contexts do not need to be considered here, as
    // these only serve to provide a default visibility (see predicate above).
    // As tag declarations could have been moved using `expose_here`, the graph
    // is used instead of iterating the declarations stored in `context`.
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

    for (clang::DeclContext::specific_decl_iterator<clang::NamedDecl>
             it(context->decls_begin()),
         end_it(context->decls_end());
         it != end_it; ++it) {
      // Nested declaration contexts that are represented in the graph are taken
      // into account above.
      // NOTE: Since e.g. `FunctionDecl`s are also `DeclContext`s, it's not
      // correct to check for `isa<DeclContext>` here.
      assert(visibilities.count(nullptr) == 0);
      bool is_also_represented_as_graph_node =
          visibilities.count(llvm::dyn_cast<clang::DeclContext>(*it));
      if (is_also_represented_as_graph_node)
        continue;
      // Do not visit implicit declarations, as these cannot have been annotated
      // explicitly by the user.  This avoids false positives on the implicit
      // `CXXRecordDecl` nested inside C++ records to represent the
      // injected-class-name.
      if (it->isImplicit())
        continue;
      if (isVisibleGivenDefaultVisibility(*it))
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
