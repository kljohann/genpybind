#pragma once

#include "genpybind/decl_context_graph.h"

namespace genpybind {

class AnnotationStorage;

/// Returns a pruned copy of `graph`, where hidden nodes are omitted.
/// This uses the effective node visibilities stored in `annotations`,
/// and consequently has to be called after visibility propagation.
/// Warnings are emitted, if any declaration context is not part of the
/// pruned tree, but is marked visible.
DeclContextGraph pruneGraph(const DeclContextGraph &graph,
                            const AnnotationStorage &annotations);

} // namespace genpybind
