// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <clang/AST/Decl.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Casting.h>

#include <cassert>
#include <memory>

namespace llvm {
template <class GraphType> struct GraphTraits;
} // namespace llvm

namespace genpybind {

class DeclContextNode;

/// A graph of nested lookup contexts that is used to calculate where and if
/// each contained context is exposed.
/// The default parent of each node is its closest semantic parent that is also
/// a lookup context; other itermediate declaration contexts are omitted.
/// The root of the graph is formed by a `TranslationUnitDecl`.
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

  /// Return whether the given `declaration` fulfills the criteria to be
  /// represented in the graph.
  static bool accepts(const clang::Decl *declaration);

  unsigned size() const { return nodes.size(); }

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
  DeclContextNode(const clang::Decl *decl) : decl(decl) {
    assert(DeclContextGraph::accepts(decl) &&
           "graph should only contain valid nodes");
    assert([](const clang::TagDecl *decl) {
      return decl == nullptr || decl->getDefinition() == decl;
    }(llvm::dyn_cast<clang::TagDecl>(decl)) &&
           "graph should only contain tag decls that are their definition");
  }

  void addChild(DeclContextNode *child) { children.push_back(child); }

  const clang::Decl *getDecl() const { return decl; }
  const clang::DeclContext *getDeclContext() const {
    return llvm::cast<clang::DeclContext>(decl);
  }

  using iterator = llvm::SmallVectorImpl<Child>::iterator;
  using const_iterator = llvm::SmallVectorImpl<Child>::const_iterator;

  /// Iterates through child nodes in deterministic but unspecified order.
  iterator begin() { return children.begin(); }
  iterator end() { return children.end(); }
  const_iterator begin() const { return children.begin(); }
  const_iterator end() const { return children.end(); }
};

} // namespace genpybind

namespace llvm {

template <> struct GraphTraits<::genpybind::DeclContextNode *> {
  using Node = ::genpybind::DeclContextNode;
  using NodeRef = Node *;
  using ChildIteratorType = Node::iterator;

  static NodeRef getEntryNode(::genpybind::DeclContextNode *graph) {
    return graph;
  }
  static ChildIteratorType child_begin(NodeRef node) { return node->begin(); }
  static ChildIteratorType child_end(NodeRef node) { return node->end(); }
};

template <> struct GraphTraits<const ::genpybind::DeclContextNode *> {
  using Node = const ::genpybind::DeclContextNode;
  using NodeRef = Node *;
  using ChildIteratorType = Node::const_iterator;

  static NodeRef getEntryNode(::genpybind::DeclContextNode *graph) {
    return graph;
  }
  static ChildIteratorType child_begin(NodeRef node) { return node->begin(); }
  static ChildIteratorType child_end(NodeRef node) { return node->end(); }
};

template <>
struct GraphTraits<::genpybind::DeclContextGraph *>
    : public GraphTraits<::genpybind::DeclContextNode *> {
  static NodeRef getEntryNode(::genpybind::DeclContextGraph *graph) {
    return graph->getRoot();
  }
  static NodeRef
  valueFromPair(::genpybind::DeclContextGraph::iterator::value_type &pair) {
    return pair.second.get();
  }
  using nodes_iterator =
      mapped_iterator<::genpybind::DeclContextGraph::iterator,
                      decltype(&valueFromPair)>;
  static nodes_iterator nodes_begin(::genpybind::DeclContextGraph *graph) {
    return nodes_iterator(graph->begin(), &valueFromPair);
  }

  static nodes_iterator nodes_end(::genpybind::DeclContextGraph *graph) {
    return nodes_iterator(graph->end(), &valueFromPair);
  }

  static unsigned size(::genpybind::DeclContextGraph *graph) {
    return graph->size();
  }
};

template <>
struct GraphTraits<const ::genpybind::DeclContextGraph *>
    : public GraphTraits<const ::genpybind::DeclContextNode *> {
  static NodeRef getEntryNode(const ::genpybind::DeclContextGraph *graph) {
    return graph->getRoot();
  }
  static NodeRef valueFromPair(
      ::genpybind::DeclContextGraph::const_iterator::value_type &pair) {
    return pair.second.get();
  }
  using nodes_iterator =
      mapped_iterator<::genpybind::DeclContextGraph::const_iterator,
                      decltype(&valueFromPair)>;
  static nodes_iterator
  nodes_begin(const ::genpybind::DeclContextGraph *graph) {
    return nodes_iterator(graph->begin(), &valueFromPair);
  }

  static nodes_iterator nodes_end(const ::genpybind::DeclContextGraph *graph) {
    return nodes_iterator(graph->end(), &valueFromPair);
  }

  static unsigned size(const ::genpybind::DeclContextGraph *graph) {
    return graph->size();
  }
};

} // namespace llvm
