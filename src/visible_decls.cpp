#include "genpybind/visible_decls.h"

#include "genpybind/annotated_decl.h"
#include "genpybind/decl_context_graph.h"

#include <clang/AST/CXXInheritance.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Sema/Lookup.h>
#include <clang/Sema/Sema.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Support/Casting.h>

using namespace genpybind;

namespace {

class ExposableDeclConsumer : public clang::VisibleDeclConsumer {
  llvm::Optional<RecordInliningPolicy> inlining_policy;
  std::vector<const clang::NamedDecl *> &decls;

  static bool isConstructor(const clang::Decl *decl) {
    if (const auto *tpl = llvm::dyn_cast<clang::FunctionTemplateDecl>(decl))
      decl = tpl->getTemplatedDecl();
    return llvm::isa<clang::CXXConstructorDecl>(decl);
  }

public:
  ExposableDeclConsumer(llvm::Optional<RecordInliningPolicy> inlining_policy,
                        std::vector<const clang::NamedDecl *> &decls)
      : inlining_policy(inlining_policy), decls(decls) {}

  bool shouldInlineDecl(clang::NamedDecl *proposed_decl) {
    // TODO: What about using declarations? Where should they be resolved?
    if (!inlining_policy.hasValue() || isConstructor(proposed_decl) ||
        llvm::isa<clang::CXXDestructorDecl>(proposed_decl))
      return false;

    const auto *parent_decl =
        llvm::dyn_cast<clang::CXXRecordDecl>(proposed_decl->getDeclContext());
    if (parent_decl == nullptr)
      return false;

    return inlining_policy->shouldInline(parent_decl);
  }

  void FoundDecl(clang::NamedDecl *proposed_decl, clang::NamedDecl *hiding,
                 clang::DeclContext *, bool in_base_class) override {
    bool is_visible_in_scope = hiding == nullptr;
    bool is_accessible = proposed_decl->getAccess() == clang::AS_public ||
                         proposed_decl->getAccess() == clang::AS_none;
    if (!is_visible_in_scope || !is_accessible)
      return;

    if (DeclContextGraph::accepts(proposed_decl))
      return;

    if (in_base_class && !shouldInlineDecl(proposed_decl))
      return;

    if (const auto *using_decl =
            llvm::dyn_cast<clang::UsingShadowDecl>(proposed_decl)) {
      proposed_decl = using_decl->getTargetDecl();
    }

    if (!proposed_decl->getLocation().isValid())
      return;

    decls.push_back(proposed_decl);
  }
};

} // namespace

RecordInliningPolicy RecordInliningPolicy::createFromAnnotation(
    const AnnotatedRecordDecl &annotated_record) {
  if (const auto *decl =
          llvm::dyn_cast<clang::CXXRecordDecl>(annotated_record.getDecl()))
    return {decl, annotated_record.inline_base, annotated_record.hide_base};
  return {};
}

RecordInliningPolicy::RecordInliningPolicy(
    const clang::CXXRecordDecl *record_decl,
    const llvm::SmallPtrSetImpl<const clang::TagDecl *> &inline_candidates,
    const llvm::SmallPtrSetImpl<const clang::TagDecl *> &hidden_bases) {
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
    return true;
  });
}

bool RecordInliningPolicy::shouldInline(const clang::TagDecl *decl) const {
  // TODO: Calls to getDefinition necessary here?
  decl = decl->getDefinition();
  return inline_bases.count(decl) != 0;
}

std::vector<const clang::NamedDecl *>
genpybind::collectVisibleDeclsFromDeclContext(
    clang::Sema &sema, const clang::DeclContext *decl_context,
    llvm::Optional<RecordInliningPolicy> inlining_policy) {
  std::vector<const clang::NamedDecl *> decls;
  ExposableDeclConsumer consumer(inlining_policy, decls);
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
