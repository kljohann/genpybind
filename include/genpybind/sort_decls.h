// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

namespace clang {
class Decl;
class SourceLocation;
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
