// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "genpybind/decl_context_graph.h"
#include "genpybind/decl_context_graph_builder.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>

namespace clang {
class DeclContext;
class Sema;
class SourceManager;
} // namespace clang

namespace genpybind {

class AnnotationStorage;

using EnclosingScopeMap =
    llvm::DenseMap<const clang::DeclContext *, const clang::DeclContext *>;

/// Determine the parent context where each graph node should be exposed.
/// With one exception this equivalent to the parent node in the tree:
/// Namespaces only introduce a scope if they have a "module" annotation, else
/// they are skipped over when determining the enclosing scope.
EnclosingScopeMap findEnclosingScopes(const DeclContextGraph &graph,
                                      const AnnotationStorage &annotations);

using EffectiveVisibilityMap = llvm::DenseMap<const clang::DeclContext *, bool>;

/// For each reachable node of `graph`, derives an "effective visibility".
/// This value is based on the effective visibility of the parent node ("default
/// visibility"), the access specifier of the declaration itself and an optional
/// "explicit visibility" that has been provided via `annotations`.  In the case
/// of moved declarations (via `expose_here`), only the new parent node is taken
/// into consideration.
EffectiveVisibilityMap
deriveEffectiveVisibility(const DeclContextGraph &graph,
                          const AnnotationStorage &annotations);

using ConstDeclContextSet = llvm::DenseSet<const clang::DeclContext *>;

/// Returns the set of reachable declaration contexts, which is implicitly
/// encoded in the domain of the effective visibility map.
ConstDeclContextSet
reachableDeclContexts(const EffectiveVisibilityMap &visibilities);

/// Inspects `graph` for unreachable cycles, which are reported
/// as invalid use of `expose_here` annotations.
/// \return whether a cycle has been detected.
bool reportExposeHereCycles(
    const DeclContextGraph &graph,
    const ConstDeclContextSet &reachable_contexts,
    const DeclContextGraphBuilder::RelocatedDeclsMap &relocated_decls,
    const clang::SourceManager &source_manager);

/// Returns the set of reachable declaration contexts, which are ancestors to at
/// least one visible, named declaration that is not a namespace.
/// While the visibility of a namespace only has an effect on the default
/// visibility of its descendants, a hidden tag declaration effectively conceals
/// the sub-tree of all contained declarations.
ConstDeclContextSet
declContextsWithVisibleNamedDecls(clang::Sema &sema,
                                  const DeclContextGraph *graph,
                                  const AnnotationStorage &annotations,
                                  const EffectiveVisibilityMap &visibilities);

/// Change visibility of namespaces, s.t. namespaces with an `only_expose_in`
/// annotation are only included, if it matches the `module_name`.
/// This also affects the visibility of nested declarations.
void hideNamespacesBasedOnExposeInAnnotation(
    const DeclContextGraph &graph, const AnnotationStorage &annotations,
    ConstDeclContextSet &contexts_with_visible_decls,
    EffectiveVisibilityMap &visibilities, llvm::StringRef module_name);

/// Returns a pruned copy of `graph`, where hidden and unreachable nodes are
/// omitted based on the passed effective node `visibilities`.
/// Hidden records and their nested contexts are omitted unconditionally.
/// For namespaces the effective visibility only serves as the default
/// visibility of the contained declarations.  As a consequence, they are
/// preserved if they recursively contain at least one visible declaration.
/// This is necessary as free functions, aliases or other declarations are not
/// represented in the declaration context graph and can/will only be exposed if
/// the corresponding parent context node is present.
DeclContextGraph
pruneGraph(const DeclContextGraph &graph,
           const ConstDeclContextSet &contexts_with_visible_decls,
           const EffectiveVisibilityMap &visibilities);

/// Emit warnings for any declaration context in `contexts_with_visible_decls`
/// that is not contained in `graph`.
void reportUnreachableVisibleDeclContexts(
    const DeclContextGraph &graph,
    const ConstDeclContextSet &contexts_with_visible_decls,
    const DeclContextGraphBuilder::RelocatedDeclsMap &relocated_decls,
    const clang::SourceManager &source_manager);

/// Returns reachable declaration contexts of `graph` in an order suitable for
/// binding generation.
/// As the context in which a declaration is exposed needs to be defined
/// before the declaration itself, nested declarations are sorted after their
/// `parents`.  In addition, exposed base classes (i.e. those that are also
/// represented in the `graph`) are exposed before derived classes, as required
/// by pybind11.
/// If the dependency graph contains a cycle, an empty sequence is returned and
/// `cycle` is set to one declaration context on this cycle.
llvm::SmallVector<const clang::DeclContext *, 0>
declContextsSortedByDependencies(const DeclContextGraph &graph,
                                 const EnclosingScopeMap &parents,
                                 const clang::SourceManager &source_manager,
                                 const clang::DeclContext **cycle);

} // namespace genpybind
