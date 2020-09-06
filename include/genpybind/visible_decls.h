#pragma once

#include <llvm/ADT/None.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/SetVector.h>

#include <vector>

namespace clang {
class CXXRecordDecl;
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

/// A set of base classes whose declarations should be "inlined" into
/// a given record.  There has to be a path with public access from
/// the record to the base class.
class RecordInliningPolicy {
  using BaseSet = llvm::SmallSetVector<const clang::TagDecl *, 1>;
  using const_iterator = BaseSet::const_iterator;

public:
  RecordInliningPolicy() = default;

  /// Create a policy for `record_decl`, where all reachable base classes also
  /// contained in `inline_candidates` are considered for inlining.
  /// The specified `hidden_bases` effectively cut the path to certain base
  /// classes, making them unreachable.
  RecordInliningPolicy(
      const clang::CXXRecordDecl *record_decl,
      const llvm::SmallPtrSetImpl<const clang::TagDecl *> &inline_candidates,
      const llvm::SmallPtrSetImpl<const clang::TagDecl *> &hidden_bases);

  static RecordInliningPolicy
  createFromAnnotation(const AnnotatedRecordDecl &annotated_record);

  /// Iterate through inline base classes in a deterministic but unspecified
  /// order.
  const_iterator begin() const { return inline_bases.begin(); }
  const_iterator end() const { return inline_bases.end(); }

  bool shouldInline(const clang::TagDecl *decl) const;

private:
  BaseSet inline_bases;
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
