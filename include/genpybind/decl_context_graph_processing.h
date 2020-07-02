#pragma once

#include <llvm/ADT/DenseMap.h>

#include "genpybind/decl_context_graph.h"

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

/// Returns a pruned copy of `graph`, where hidden nodes are omitted
/// based on the passed effective node `visibilities`.
/// Hidden records and their nested contexts are omitted unconditionally.
/// For namespaces and unnamed declaration contexts the effective visibility
/// only serves as the default visibility of the contained declarations.  As a
/// consequence, they are preserved if they recursively contain at least one
/// visible declaration.  This is necessary as free functions, aliases or other
/// declarations are not represented in the declaration context graph and
/// can/will only be exposed if the corresponding parent context node is
/// present.
/// Warnings are emitted, if any declaration context is not part of the
/// pruned tree, but is marked visible.
DeclContextGraph pruneGraph(const DeclContextGraph &graph,
                            const AnnotationStorage &annotations,
                            const EffectiveVisibilityMap &visibilities);

} // namespace genpybind
