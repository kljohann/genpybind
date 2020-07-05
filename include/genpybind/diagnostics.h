#pragma once

#include <clang/AST/Decl.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>

#include <string>

namespace genpybind {

std::string getNameForDisplay(const clang::Decl *decl);

class Diagnostics {
  clang::DiagnosticsEngine &engine;

public:
  enum class Kind {
    AlreadyExposedElsewhereError,
    AnnotationInvalidForDeclKindError,
    AnnotationInvalidSpellingError,
    AnnotationWrongArgumentTypeError,
    AnnotationWrongNumberOfArgumentsError,
    ConflictingAnnotationsError,
    ExposeHereCycleError,
    IgnoringQualifiersOnAliasWarning,
    PreviouslyExposedHereNote,
    UnreachableDeclContextWarning,
    UnsupportedAliasTargetError,
  };

  Diagnostics(clang::DiagnosticsEngine &engine) : engine(engine) {}

  static clang::DiagnosticBuilder report(const clang::Decl *decl, Kind kind);
  clang::DiagnosticBuilder report(clang::SourceLocation loc, Kind kind);
};

} // namespace genpybind
