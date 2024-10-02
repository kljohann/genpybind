// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/sort_decls.h"

#include <clang/AST/Decl.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

using namespace genpybind;

IsBeforeInTranslationUnit::IsBeforeInTranslationUnit(
    const clang::SourceManager &source_manager)
    : source_manager(source_manager) {}

bool IsBeforeInTranslationUnit::operator()(clang::SourceLocation lhs,
                                           clang::SourceLocation rhs) const {
  return source_manager.isBeforeInTranslationUnit(lhs, rhs);
}

bool IsBeforeInTranslationUnit::operator()(const clang::Decl *lhs,
                                           const clang::Decl *rhs) const {
  // TODO: getBeginLoc? Macros?
  return operator()(lhs->getLocation(), rhs->getLocation());
}
