#include "genpybind/diagnostics.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/QualTypeNames.h>

using namespace genpybind;

std::string genpybind::getNameForDisplay(const clang::Decl *decl) {
  if (const auto *type_decl = llvm::dyn_cast<clang::TypeDecl>(decl)) {
    const clang::ASTContext &context = type_decl->getASTContext();
    const clang::QualType qual_type = context.getTypeDeclType(type_decl);
    return clang::TypeName::getFullyQualifiedName(qual_type, context,
                                                  context.getPrintingPolicy());
  }

  if (const auto *named_decl = llvm::dyn_cast<clang::NamedDecl>(decl))
    return named_decl->getQualifiedNameAsString();

  return "";
}

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
  case Kind::ConflictingAnnotationsError:
    return engine.getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "'%0' and '%1' cannot be used at the same time");
  case Kind::ExposeHereCycleError:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Error,
                                  "'expose_here' annotations form a cycle");
  case Kind::IgnoringQualifiersOnAliasWarning:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Warning,
                                  "Ignoring qualifiers on alias");
  case Kind::PreviouslyExposedHereNote:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Note,
                                  "Previously exposed here");
  case Kind::UnreachableDeclContextWarning:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Warning,
                                  "Declaration context '%0' contains 'visible' "
                                  "declarations but is not exposed");
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
