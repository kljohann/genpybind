#include "genpybind/decl_context_graph_builder.h"

#include "genpybind/annotated_decl.h"
#include "genpybind/annotations/annotation.h"
#include "genpybind/diagnostics.h"
#include "genpybind/lookup_context_collector.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <llvm/ADT/None.h>
#include <llvm/Support/Casting.h>

#include <cassert>
#include <utility>
#include <vector>

using namespace genpybind;

using annotations::Annotation;
using annotations::AnnotationKind;

static const clang::Decl *
findLookupContextDecl(const clang::DeclContext *decl_context) {
  while (decl_context != nullptr && !decl_context->isLookupContext())
    decl_context = decl_context->getParent();
  return llvm::dyn_cast_or_null<clang::Decl>(decl_context);
}

auto DeclContextGraphBuilder::aliasTarget(const clang::TypedefNameDecl *decl)
    -> const clang::TagDecl * {
  const clang::TagDecl *target_decl = decl->getUnderlyingType()->getAsTagDecl();
  assert(target_decl != nullptr &&
         "type aliases can only be used with tag type targets");
  return target_decl->getDefinition();
}

bool DeclContextGraphBuilder::addEdgeForExposeHereAlias(
    const clang::TypedefNameDecl *decl) {
  const clang::Decl *parent = findLookupContextDecl(decl->getDeclContext());
  const clang::TagDecl *target_decl = aliasTarget(decl);

  auto inserted = relocated_decls.try_emplace(target_decl, decl);
  if (!inserted.second) {
    Diagnostics::report(decl, Diagnostics::Kind::AlreadyExposedElsewhereError)
        << target_decl->getNameAsString();
    Diagnostics::report(inserted.first->getSecond(),
                        Diagnostics::Kind::PreviouslyExposedHereNote)
        << target_decl->getNameAsString();
    return false;
  }

  graph.getOrInsertNode(parent)->addChild(graph.getOrInsertNode(target_decl));
  return true;
}

llvm::Optional<DeclContextGraph> DeclContextGraphBuilder::buildGraph() {
  clang::DiagnosticErrorTrap trap{
      translation_unit->getASTContext().getDiagnostics()};
  LookupContextCollector visitor(annotations);
  visitor.TraverseDecl(translation_unit);

  // Bail out if there were errors during the first traversal of the AST,
  // e.g. due to invalid annotations.
  if (trap.hasErrorOccurred())
    return llvm::None;

  for (const clang::TypedefNameDecl *alias_decl : visitor.aliases) {
    const auto *annotated = llvm::dyn_cast_or_null<AnnotatedTypedefNameDecl>(
        annotations.get(alias_decl));
    assert(annotated != nullptr &&
           "only aliases with annotations are collected");
    if (!annotated->encourage && !annotated->expose_here)
      continue;
    const clang::TagDecl *target_decl = aliasTarget(alias_decl);
    AnnotatedDecl *annotated_target = annotations.getOrInsert(target_decl);
    assert(annotated_target != nullptr);
    if (annotated->encourage)
      annotated_target->processAnnotation(Annotation(AnnotationKind::Visible));
    if (annotated->expose_here && addEdgeForExposeHereAlias(alias_decl))
      annotated->propagateAnnotations(*annotated_target);
  }

  // Bail out if establishing "expose_here" aliases failed, e.g. due to
  // multiple such aliases for one declaration.
  if (trap.hasErrorOccurred())
    return llvm::None;

  for (const clang::DeclContext *decl_context : visitor.lookup_contexts) {
    const auto *decl = llvm::dyn_cast<clang::Decl>(decl_context);
    // Do not add original parent-child-edge to graph if decl has been moved.
    if (relocated_decls.count(decl) != 0)
      continue;
    // Expose declarations below their semantic parent.
    const clang::Decl *parent = findLookupContextDecl(decl->getDeclContext());
    graph.getOrInsertNode(parent)->addChild(graph.getOrInsertNode(decl));
  }

  return std::move(graph);
}
