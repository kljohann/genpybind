#include "genpybind/decl_context_graph.h"

using namespace genpybind;

DeclContextGraph::DeclContextGraph(const clang::TranslationUnitDecl *decl)
    : nodes(), root(getOrInsertNode(decl)) {}

DeclContextNode *DeclContextGraph::getNode(const clang::Decl *decl) const {
  auto it = nodes.find(decl);
  if (it != nodes.end())
    return it->second.get();
  return nullptr;
}

DeclContextNode *DeclContextGraph::getOrInsertNode(const clang::Decl *decl) {
  std::unique_ptr<DeclContextNode> &node_slot = nodes[decl];
  if (!node_slot)
    node_slot = std::make_unique<DeclContextNode>(decl);
  return node_slot.get();
}
