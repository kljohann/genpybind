#pragma once

#include <clang/AST/Decl.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>

#include <memory>

namespace genpybind {

class DeclContextNode;

/// A graph of lexically nested declaration contexts that is used to calculate
/// where and if each contained context is exposed.  Only contexts that can
/// contain a completely defined `TagDecl` with external linkage are considered.
/// The root of the graph is formed by a `TranslationUnitDecl`.
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
///
/// Based on the effective visibility the graph is pruned: Hidden branches
/// are removed.
///
/// Finally, the graph is used to topologically sort the declarations in order
/// to expose them in the right order.  During this traversal, nodes not
/// connected to the tree are discovered and reported.
class DeclContextGraph {
  using NodeStorage =
      llvm::DenseMap<const clang::Decl *, std::unique_ptr<DeclContextNode>>;

  /// Map that owns the graph nodes and provides lookup by AST nodes.
  NodeStorage nodes;

  /// Pointer to `TranslationUnitDecl` that forms the root of the hierarchy.
  DeclContextNode *root;

public:
  DeclContextGraph(const clang::TranslationUnitDecl *decl);

  DeclContextNode *getRoot() const { return root; }
  DeclContextNode *getNode(const clang::Decl *decl) const;
  DeclContextNode *getOrInsertNode(const clang::Decl *decl);

  using iterator = NodeStorage::iterator;
  using const_iterator = NodeStorage::const_iterator;

  /// Iterates through all nodes in non-deterministic order.
  iterator begin() { return nodes.begin(); }
  iterator end() { return nodes.end(); }
  const_iterator begin() const { return nodes.begin(); }
  const_iterator end() const { return nodes.end(); }
};

class DeclContextNode {
public:
  using Child = DeclContextNode *;

private:
  const clang::Decl *decl;
  llvm::SmallVector<Child, 10> children;

public:
  DeclContextNode(const clang::Decl *decl) : decl(decl) {}

  void addChild(DeclContextNode *child) { children.push_back(child); }

  const clang::Decl *getDecl() const { return decl; }

  using iterator = llvm::SmallVectorImpl<Child>::iterator;
  using const_iterator = llvm::SmallVectorImpl<Child>::const_iterator;

  /// Iterates through child nodes in deterministic but unspecified order.
  iterator begin() { return children.begin(); }
  iterator end() { return children.end(); }
  const_iterator begin() const { return children.begin(); }
  const_iterator end() const { return children.end(); }
};

} // namespace genpybind
