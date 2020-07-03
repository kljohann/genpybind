#pragma once

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>

#include "genpybind/decl_context_graph.h"
#include "genpybind/decl_context_graph_builder.h"

namespace genpybind {

class AnnotationStorage;

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
    const DeclContextGraphBuilder::RelocatedDeclsMap &relocated_decls);

/// Returns the set of reachable declaration contexts, which are ancestors to at
/// least one visible, named declaration that is not a namespace.
/// While the visibility of a namespace or unnamed declaraiton context only
/// has an effect on the default visibility of its descendants, a hidden tag
/// declaration effectively conceals the sub-tree of all contained declarations.
ConstDeclContextSet
declContextsWithVisibleNamedDecls(const DeclContextGraph *graph,
                                  const AnnotationStorage &annotations,
                                  const EffectiveVisibilityMap &visibilities);

/// Returns a pruned copy of `graph`, where hidden and unreachable nodes are
/// omitted based on the passed effective node `visibilities`.
/// Hidden records and their nested contexts are omitted unconditionally.
/// For namespaces and unnamed declaration contexts the effective visibility
/// only serves as the default visibility of the contained declarations.  As a
/// consequence, they are preserved if they recursively contain at least one
/// visible declaration.  This is necessary as free functions, aliases or other
/// declarations are not represented in the declaration context graph and
/// can/will only be exposed if the corresponding parent context node is
/// present.
DeclContextGraph
pruneGraph(const DeclContextGraph &graph,
           const ConstDeclContextSet &contexts_with_visible_decls,
           const EffectiveVisibilityMap &visibilities);

/// Emit warnings for any declaration context in `contexts_with_visible_decls`
/// that is not contained in `graph`.
void reportUnreachableVisibleDeclContexts(
    const DeclContextGraph &graph,
    const ConstDeclContextSet &contexts_with_visible_decls,
    const DeclContextGraphBuilder::RelocatedDeclsMap &relocated_decls);

} // namespace genpybind
