#pragma once

#include <clang/Basic/Diagnostic.h>

#include <string>

namespace clang {
class Decl;
class SourceLocation;
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
    AnnotationsNeedToMatchCanonicalDeclError,
    ConflictingAnnotationsError,
    ExposeHereCycleError,
    IgnoringQualifiersOnAliasWarning,
    PreviouslyExposedHereNote,
    PropertyAlreadyDefinedError,
    PropertyHasNoGetterError,
    UnreachableDeclContextWarning,
    UnsupportedAliasTargetError,
  };

  static clang::DiagnosticBuilder report(const clang::Decl *decl, Kind kind);
  static clang::DiagnosticBuilder report(const clang::Decl *decl,
                                         unsigned diag_id);
};

} // namespace genpybind
