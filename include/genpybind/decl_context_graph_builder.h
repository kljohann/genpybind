#pragma once

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallPtrSet.h>

#include "genpybind/annotated_decl.h"
#include "genpybind/decl_context_graph.h"

namespace genpybind {

/// Builds a graph of declaration contexts from the AST.  As described for
/// DeclContextGraph, this graph is used to determine where and in which order
/// to expose the different declaration contexts.
///
/// A `TagDecl` can only be exposed as a single location.  By default, the
/// parent node of a `TagDecl` corresponds to the node of the lexical
/// declaration context, where it was declared and defined.  However, they can
/// also be exposed at a different location by moving the corresponding
/// sub-graph to a new parent.  This is effected by typedef or type alias
/// declarations that have an `expose_here` annotation.  Their underlying type
/// indicates the `TagDecl` to be moved and their lexical declaration context
/// indicates the new parent node.  Incorrect use of this feature can lead to
/// loops of nodes that are no longer attached to the main tree.  This is
/// detected and reported to the user.  There can only be a single `expose_here`
/// annotation for each `TagDecl`.
///
/// Relocation of nodes via these aliases is implemented by processing the
/// aliases before the declaration contexts themselves and later ignoring
/// declaration contexts if a corresponding node is already found in the graph.
///
/// For each declaration, an "effective visibility" is calculated based on the
/// effective visibility of its parent node ("default visibility"), its own
/// access specifier and an optional "explicit visibility" that has been
/// provided via annotation attributes attached to the corresponding
/// declaration. In the case of moved declarations (via `expose_here`),
/// only the new parent node is taken into consideration.
class DeclContextGraphBuilder {
  using ConstNodeSet = llvm::SmallPtrSetImpl<const DeclContextNode *>;

  clang::TranslationUnitDecl *translation_unit;
  AnnotationStorage &annotations;
  DeclContextGraph graph;
  llvm::DenseMap<const clang::Decl *, const clang::TypedefNameDecl *>
      moved_previously;

  const clang::TagDecl *aliasTarget(const clang::TypedefNameDecl *decl);

  bool addEdgeForExposeHereAlias(const clang::TypedefNameDecl *decl);
  bool reportExposeHereCycles(const ConstNodeSet &reachable_nodes) const;

public:
  DeclContextGraphBuilder(AnnotationStorage &annotations,
                          clang::TranslationUnitDecl *decl)
      : translation_unit(decl), annotations(annotations), graph(decl) {}

  bool buildGraph();

  /// Computes the effective visibility for all reachable declaration contexts.
  /// Any unreachable nodes are inspected for `expose_here` cycles, which are
  /// consequently reported as errors.
  bool propagateVisibility();

  const DeclContextGraph& getGraph() const { return graph; }
};

} // namespace genpybind
