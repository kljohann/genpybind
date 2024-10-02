// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <llvm/ADT/SmallPtrSet.h>

#include <optional>
#include <vector>

namespace clang {
class CXXRecordDecl;
class DeclContext;
class NamedDecl;
class Sema;
class TagDecl;
} // namespace clang

namespace genpybind {
class AnnotationStorage;

/// A set of base classes whose declarations should be "inlined" into
/// a given record.  There has to be a path with public access from
/// the record to the base class.
class RecordInliningPolicy {
  using BaseSet = llvm::SmallPtrSet<const clang::TagDecl *, 1>;

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
  createFromAnnotations(const AnnotationStorage &annotations,
                        const clang::CXXRecordDecl *record_decl);

  bool shouldInline(const clang::TagDecl *decl) const;
  bool shouldHide(const clang::TagDecl *decl) const;

private:
  BaseSet inline_bases;
  BaseSet hide_bases;
};

/// Return all visible (in the language sense of name hiding) named declarations
/// in the specified `decl_context`, that are not represented in the decl
/// context graph.  If `decl_context` is a record, only public declarations
/// are considered.
/// Optionally specify an `inlining_policy` that describes whether visible
/// declarations inherited from base classes should be included.
std::vector<const clang::NamedDecl *> collectVisibleDeclsFromDeclContext(
    clang::Sema &sema, const clang::DeclContext *decl_context,
    std::optional<RecordInliningPolicy> inlining_policy = std::nullopt);

} // namespace genpybind
