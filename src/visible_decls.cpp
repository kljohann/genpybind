// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/visible_decls.h"

#include "genpybind/annotated_decl.h"
#include "genpybind/decl_context_graph.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/CXXInheritance.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Sema/Lookup.h>
#include <clang/Sema/Sema.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Casting.h>

#include <utility>
#include <vector>

using namespace genpybind;

namespace {

class ExposableDeclConsumer : public clang::VisibleDeclConsumer {
  std::optional<RecordInliningPolicy> inlining_policy;
  std::vector<const clang::NamedDecl *> &decls;

  static bool isConstructor(const clang::Decl *decl) {
    if (const auto *tpl = llvm::dyn_cast<clang::FunctionTemplateDecl>(decl))
      decl = tpl->getTemplatedDecl();
    return llvm::isa<clang::CXXConstructorDecl>(decl);
  }

public:
  ExposableDeclConsumer(std::optional<RecordInliningPolicy> inlining_policy,
                        std::vector<const clang::NamedDecl *> &decls)
      : inlining_policy(std::move(inlining_policy)), decls(decls) {}

  bool shouldInlineDecl(clang::NamedDecl *proposed_decl) {
    // TODO: What about using declarations? Where should they be resolved?
    if (!inlining_policy.has_value() || isConstructor(proposed_decl) ||
        llvm::isa<clang::CXXDestructorDecl>(proposed_decl))
      return false;

    const auto *parent_decl =
        llvm::dyn_cast<clang::CXXRecordDecl>(proposed_decl->getDeclContext());
    if (parent_decl == nullptr)
      return false;

    return inlining_policy->shouldInline(parent_decl);
  }

  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  void FoundDecl(clang::NamedDecl *proposed_decl, clang::NamedDecl *hiding,
                 clang::DeclContext *, bool in_base_class) override {
    bool is_visible_in_scope = hiding == nullptr;
    bool is_accessible = proposed_decl->getAccess() == clang::AS_public ||
                         proposed_decl->getAccess() == clang::AS_none;
    if (!is_visible_in_scope || !is_accessible)
      return;

    if (in_base_class && !shouldInlineDecl(proposed_decl))
      return;

    if (const auto *using_decl =
            llvm::dyn_cast<clang::UsingShadowDecl>(proposed_decl)) {
      proposed_decl = using_decl->getTargetDecl();
    }

    if (DeclContextGraph::accepts(proposed_decl))
      return;

    if (!proposed_decl->getLocation().isValid())
      return;

    decls.push_back(proposed_decl);
  }
};

} // namespace

RecordInliningPolicy RecordInliningPolicy::createFromAnnotations(
    const AnnotationStorage &annotations,
    const clang::CXXRecordDecl *record_decl) {
  llvm::SmallVector<const clang::TagDecl *, 1> remaining{record_decl};
  llvm::SmallPtrSet<const clang::TagDecl *, 1> inline_candidates;
  llvm::SmallPtrSet<const clang::TagDecl *, 1> hidden_bases;

  // If a base class is a candidate for inlining, treat its `inline_base` and
  // `hide_base` annotations as if they were present on `record_decl`.
  while (!remaining.empty()) {
    const clang::TagDecl *decl = remaining.back();
    remaining.pop_back();
    if (const auto attrs = annotations.get<RecordDeclAttrs>(decl)) {
      hidden_bases.insert(attrs->hide_base.begin(), attrs->hide_base.end());
      for (const clang::TagDecl *base : attrs->inline_base) {
        auto result = inline_candidates.insert(base);
        if (result.second)
          remaining.push_back(base);
      }
    }
  }

  return {record_decl, inline_candidates, hidden_bases};
}

RecordInliningPolicy::RecordInliningPolicy(
    const clang::CXXRecordDecl *record_decl,
    const llvm::SmallPtrSetImpl<const clang::TagDecl *> &inline_candidates,
    const llvm::SmallPtrSetImpl<const clang::TagDecl *> &hidden_bases)
    : hide_bases(hidden_bases.begin(), hidden_bases.end()) {
  auto is_hidden_base = [&](const clang::CXXBasePathElement &element) -> bool {
    const auto *base_decl = element.Base->getType()->getAsCXXRecordDecl();
    base_decl = base_decl->getDefinition();
    return hidden_bases.count(base_decl) != 0;
  };

  auto is_valid_path = [&](const clang::CXXBasePath &path) -> bool {
    return path.Access == clang::AS_public &&
           !llvm::any_of(path, is_hidden_base);
  };

  auto should_inline = [&](const clang::CXXRecordDecl *base_decl) -> bool {
    base_decl = base_decl->getDefinition();
    if (inline_candidates.count(base_decl) == 0)
      return false;

    clang::CXXBasePaths paths;
    record_decl->isDerivedFrom(base_decl, paths);
    return llvm::any_of(paths, is_valid_path);
  };

  // Use `forallBases` in order to provide a deterministic iteration order.
  record_decl->forallBases([&](const clang::CXXRecordDecl *base_decl) -> bool {
    if (should_inline(base_decl))
      inline_bases.insert(base_decl);
    return true; // continue visiting other bases
  });
}

bool RecordInliningPolicy::shouldInline(const clang::TagDecl *decl) const {
  // TODO: Calls to getDefinition necessary here?
  decl = decl->getDefinition();
  return inline_bases.count(decl) != 0;
}

bool RecordInliningPolicy::shouldHide(const clang::TagDecl *decl) const {
  // TODO: Calls to getDefinition necessary here?
  decl = decl->getDefinition();
  return hide_bases.count(decl) != 0;
}

std::vector<const clang::NamedDecl *>
genpybind::collectVisibleDeclsFromDeclContext(
    clang::Sema &sema, const clang::DeclContext *decl_context,
    std::optional<RecordInliningPolicy> inlining_policy) {
  std::vector<const clang::NamedDecl *> decls;
  ExposableDeclConsumer consumer(std::move(inlining_policy), decls);
  // Include global scope *iff* looking up decls in the TU decl context.
  bool include_global_scope = llvm::dyn_cast<clang::Decl>(decl_context) ==
                              sema.getASTContext().getTranslationUnitDecl();
  sema.LookupVisibleDecls(const_cast<clang::DeclContext *>(decl_context),
                          clang::Sema::LookupOrdinaryName, consumer,
                          /*IncludeGlobalScope=*/include_global_scope,
                          /*IncludeDependentBases=*/false,
                          /*LoadExternal=*/true);
  return decls;
}
