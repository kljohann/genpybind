#pragma once

#include <llvm/ADT/None.h>
#include <llvm/ADT/Optional.h>

#include <vector>

namespace clang {
class DeclContext;
class NamedDecl;
class Sema;
class TagDecl;
} // namespace clang

namespace llvm {
template <typename T> class SmallPtrSetImpl;
} // namespace llvm

namespace genpybind {

class AnnotatedRecordDecl;

struct RecordInliningPolicy {
  /*implicit*/ RecordInliningPolicy(
      const AnnotatedRecordDecl &annotated_record);

  /// Set of base classes whose declarations should be "inlined" into
  /// a given record.  There has to be a path with public access from
  /// the record to the base class.
  const llvm::SmallPtrSetImpl<const clang::TagDecl *> &inline_base;
  /// Set of base classes that should be hidden from the hierarchy,
  /// effectively cutting the path considered for `inline_base`.
  const llvm::SmallPtrSetImpl<const clang::TagDecl *> &hide_base;
};

/// Return all visible (in the language sense of name hiding) named declarations
/// in the specified `decl_context`, that are not represented in the decl
/// context graph.  If `decl_context` is a record, only public declarations
/// are considered.
/// Optionally specify an `inlining_policy` that describes whether visible
/// declarations inherited from base classes should be included.
std::vector<const clang::NamedDecl *> collectVisibleDeclsFromDeclContext(
    clang::Sema &sema, const clang::DeclContext *decl_context,
    llvm::Optional<RecordInliningPolicy> inlining_policy = llvm::None);

} // namespace genpybind
