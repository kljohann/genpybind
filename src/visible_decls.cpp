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

#include <cassert>

using namespace genpybind;

namespace {

class ExposableDeclConsumer : public clang::VisibleDeclConsumer {
  const clang::DeclContext *context;
  llvm::Optional<RecordInliningPolicy> inlining_policy;
  std::vector<const clang::NamedDecl *> &decls;

  static bool isConstructor(const clang::Decl *decl) {
    if (const auto *tpl = llvm::dyn_cast<clang::FunctionTemplateDecl>(decl))
      decl = tpl->getTemplatedDecl();
    return llvm::isa<clang::CXXConstructorDecl>(decl);
  }

public:
  ExposableDeclConsumer(const clang::DeclContext *context,
                        llvm::Optional<RecordInliningPolicy> inlining_policy,
                        std::vector<const clang::NamedDecl *> &decls)
      : context(context), inlining_policy(inlining_policy), decls(decls) {
    assert(context != nullptr);
  }

  bool shouldInlineDecl(clang::NamedDecl *proposed_decl) {
    // TODO: What about using declarations? Where should they be resolved?
    if (!inlining_policy.hasValue() || isConstructor(proposed_decl) ||
        llvm::isa<clang::CXXDestructorDecl>(proposed_decl))
      return false;

    const auto *record_decl = llvm::dyn_cast<clang::CXXRecordDecl>(context);
    if (record_decl == nullptr)
      return false;

    const auto *parent_decl =
        llvm::dyn_cast<clang::CXXRecordDecl>(proposed_decl->getDeclContext());
    if (parent_decl == nullptr)
      return false;

    // TODO: Calls to getDefinition necessary here?
    parent_decl = parent_decl->getDefinition();

    if (inlining_policy->inline_base.count(parent_decl) == 0)
      return false;

    // Check if there is a public path to this base class, which does not
    // contain a hidden base class.

    auto is_hidden_base =
        [&](const clang::CXXBasePathElement &element) -> bool {
      const auto *base_decl = element.Base->getType()->getAsCXXRecordDecl();
      base_decl = base_decl->getDefinition();
      return inlining_policy->hide_base.count(base_decl) != 0;
    };

    auto is_valid_path = [&](const clang::CXXBasePath &path) -> bool {
      return path.Access == clang::AS_public &&
             !llvm::any_of(path, is_hidden_base);
    };

    clang::CXXBasePaths paths;
    record_decl->isDerivedFrom(parent_decl, paths);
    return llvm::any_of(paths, is_valid_path);
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
    decls.push_back(proposed_decl);
  }
};

} // namespace

RecordInliningPolicy::RecordInliningPolicy(
    const AnnotatedRecordDecl &annotated_record)
    : inline_base(annotated_record.inline_base),
      hide_base(annotated_record.hide_base) {}

std::vector<const clang::NamedDecl *>
genpybind::collectVisibleDeclsFromDeclContext(
    clang::Sema &sema, const clang::DeclContext *decl_context,
    llvm::Optional<RecordInliningPolicy> inlining_policy) {
  std::vector<const clang::NamedDecl *> decls;
  ExposableDeclConsumer consumer(decl_context, inlining_policy, decls);
  sema.LookupVisibleDecls(const_cast<clang::DeclContext *>(decl_context),
                          clang::Sema::LookupOrdinaryName, consumer,
                          /*IncludeGlobalScope=*/false,
                          /*IncludeDependentBases=*/false,
                          /*LoadExternal=*/true);
  return decls;
}
