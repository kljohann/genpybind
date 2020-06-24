#include "genpybind/diagnostics.h"

#include <clang/AST/ASTContext.h>

using namespace genpybind;

static unsigned getCustomDiagID(clang::DiagnosticsEngine &engine,
                                Diagnostics::Kind kind) {
  using Kind = Diagnostics::Kind;
  switch (kind) {
  case Kind::AlreadyExposedElsewhereError:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Error,
                                  "'%0' has already been exposed elsewhere");
  case Kind::AnnotationInvalidForDeclKindError:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Error,
                                  "Invalid annotation for %0: %1");
  case Kind::AnnotationInvalidSpellingError:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Error,
                                  "Invalid spelling in '%0' annotation: '%1'");
  case Kind::AnnotationWrongArgumentTypeError:
    return engine.getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "Wrong type of argument for '%0' annotation: %1");
  case Kind::AnnotationWrongNumberOfArgumentsError:
    return engine.getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "Wrong number of arguments for '%0' annotation");
  case Kind::ExposeHereCycleError:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Error,
                                  "'expose_here' annotations form a cycle");
  case Kind::IgnoringQualifiersOnAliasWarning:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Warning,
                                  "Ignoring qualifiers on alias");
  case Kind::PreviouslyExposedHereNote:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Note,
                                  "Previously exposed here");
  case Kind::UnsupportedAliasTargetError:
    return engine.getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "Annotated alias has unsupported target kind");
  }
  llvm_unreachable("Unknown diagnostic.");
}

auto Diagnostics::report(const clang::Decl *decl, Kind kind)
    -> clang::DiagnosticBuilder {
  clang::ASTContext &context = decl->getASTContext();
  return Diagnostics(context.getDiagnostics()).report(decl->getLocation(), kind)
         << decl->getSourceRange();
}

auto Diagnostics::report(clang::SourceLocation loc, Kind kind)
    -> clang::DiagnosticBuilder {
  return engine.Report(loc, getCustomDiagID(engine, kind));
}
