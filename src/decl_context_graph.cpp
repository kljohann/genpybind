// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/decl_context_graph.h"

using namespace genpybind;

DeclContextGraph::DeclContextGraph(const clang::TranslationUnitDecl *decl)
    : root(getOrInsertNode(decl)) {}

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
  const auto *ctx = llvm::dyn_cast_or_null<clang::DeclContext>(declaration);
  return ctx != nullptr && ctx->isLookupContext();
}
