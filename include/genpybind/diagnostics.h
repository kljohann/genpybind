// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <clang/Basic/Diagnostic.h>

#include <string>

namespace clang {
class Decl;
} // namespace clang

namespace genpybind {

std::string getNameForDisplay(const clang::Decl *decl);

struct Diagnostics {
  enum class Kind {
    AlreadyExposedElsewhereError,
    AnnotationContainsUnknownBaseTypeWarning,
    AnnotationIncompatibleSignatureError,
    AnnotationInvalidArgumentSpecifierError,
    AnnotationInvalidForDeclKindError,
    AnnotationInvalidSpellingError,
    AnnotationWrongArgumentTypeError,
    AnnotationWrongNumberOfArgumentsError,
    AnnotationsNeedToMatchFirstDeclError,
    ConflictingAnnotationsError,
    ExposeHereCycleError,
    InvalidAssumptionWarning,
    IgnoringQualifiersOnAliasWarning,
    OnlyGlobalScopeAllowedError,
    PreviouslyExposedHereNote,
    PropertyAlreadyDefinedError,
    PropertyHasNoGetterError,
    TrailingParametersError,
    UnreachableDeclContextWarning,
    UnsupportedAliasTargetError,
  };

  static clang::DiagnosticBuilder report(const clang::Decl *decl, Kind kind);
  static clang::DiagnosticBuilder report(const clang::Decl *decl,
                                         unsigned diag_id);
};

} // namespace genpybind
