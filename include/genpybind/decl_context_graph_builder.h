// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "genpybind/decl_context_graph.h"

#include <llvm/ADT/DenseMap.h>

#include <optional>

namespace clang {
class Decl;
class TranslationUnitDecl;
class TypedefNameDecl;
} // namespace clang

namespace genpybind {
class AnnotationStorage;

/// Builds a graph of declaration contexts from the AST.  As described for
/// DeclContextGraph, this graph is used to determine where and in which order
/// to expose the different declaration contexts.
///
/// A `TagDecl` can only be exposed at a single location.  By default, the
/// parent node of a `TagDecl` corresponds to the closest semantic parent
/// context that is also a lookup context and contains the definition of the
/// type.  However, it can also be exposed at a different location by moving the
/// corresponding sub-graph to a new parent.  This is effected by typedef or
/// type alias declarations that have an `expose_here` annotation.
/// Their underlying type indicates the `TagDecl` to be moved and their semantic
/// declaration context indicates the new parent node.  Incorrect use of this
/// feature can lead to loops of nodes that are no longer attached to the main
/// tree.  This is detected and reported to the user.  There can only be
/// a single `expose_here` annotation for each `TagDecl`.
///
/// Relocation of nodes via these aliases is implemented by processing the
/// aliases before the declaration contexts themselves and later ignoring
/// declaration contexts if a corresponding node is already found in the graph.
class DeclContextGraphBuilder {
public:
  using RelocatedDeclsMap =
      llvm::DenseMap<const clang::Decl *, const clang::TypedefNameDecl *>;

private:
  clang::TranslationUnitDecl *translation_unit;
  AnnotationStorage &annotations;
  DeclContextGraph graph;
  RelocatedDeclsMap relocated_decls;

  bool addEdgeForExposeHereAlias(const clang::TypedefNameDecl *decl);

public:
  DeclContextGraphBuilder(AnnotationStorage &annotations,
                          clang::TranslationUnitDecl *decl)
      : translation_unit(decl), annotations(annotations), graph(decl) {}

  /// Returns a map of moved decls to the `expose_here` alias that was
  /// responsible for its relocation.
  const RelocatedDeclsMap &getRelocatedDecls() const { return relocated_decls; }

  /// Builds a graph of declaration contexts for the given translation unit.
  /// Annotations for all declarations are collected in the provided
  /// `AnnotationStorage` instance.
  std::optional<DeclContextGraph> buildGraph();
};

} // namespace genpybind
