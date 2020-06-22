#pragma once

#include <clang/AST/Decl.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/GraphTraits.h>
#include <llvm/ADT/SmallVector.h>

#include <memory>

namespace genpybind {

class DeclContextNode;

/// A graph of lexically nested declaration contexts that is used to calculate
/// where and if each contained context is exposed.  Only contexts that can
/// contain a completely defined `TagDecl` with external linkage are considered.
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
