#include "genpybind/decl_context_graph.h"

#include <clang/AST/DeclCXX.h>

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
  assert(accepts(decl) && "invalid declaration kind added to graph");
  std::unique_ptr<DeclContextNode> &node_slot = nodes[decl];
  if (!node_slot)
    node_slot = std::make_unique<DeclContextNode>(decl);
  return node_slot.get();
}

bool DeclContextGraph::accepts(const clang::Decl *declaration) {
  return declaration != nullptr && llvm::isa<clang::DeclContext>(declaration) &&
         (llvm::isa<clang::TagDecl>(declaration) ||
          llvm::isa<clang::NamespaceDecl>(declaration) ||
          llvm::isa<clang::LinkageSpecDecl>(declaration) ||
          llvm::isa<clang::ExportDecl>(declaration) ||
          llvm::isa<clang::TranslationUnitDecl>(declaration));
}
