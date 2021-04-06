#pragma once

#include <clang/Basic/SourceLocation.h>

namespace clang {
class Decl;
class SourceManager;
} // namespace clang

namespace genpybind {

class IsBeforeInTranslationUnit {
  const clang::SourceManager &source_manager;

public:
  explicit IsBeforeInTranslationUnit(
      const clang::SourceManager &source_manager);

  bool operator()(clang::SourceLocation lhs, clang::SourceLocation rhs) const;

  bool operator()(const clang::Decl *lhs, const clang::Decl *rhs) const;
};

} // namespace genpybind
