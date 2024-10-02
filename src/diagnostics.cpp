// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/diagnostics.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/QualTypeNames.h>
#include <clang/AST/Type.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ErrorHandling.h>

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
  case Kind::AnnotationContainsUnknownBaseTypeWarning:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Warning,
                                  "Unknown base type in '%0' annotation");
  case Kind::AnnotationIncompatibleSignatureError:
    return engine.getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "Signature of %0 is incompatible with '%1' annotation");
  case Kind::AnnotationInvalidArgumentSpecifierError:
    return engine.getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "Invalid argument specifier in '%0' annotation: '%1'");
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
  case Kind::AnnotationsNeedToMatchFirstDeclError:
    return engine.getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "Annotations need to match those of the first declaration");
  case Kind::ConflictingAnnotationsError:
    return engine.getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "'%0' and '%1' cannot be used at the same time");
  case Kind::ExposeHereCycleError:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Error,
                                  "'expose_here' annotations form a cycle");
  case Kind::InvalidAssumptionWarning:
    return engine.getCustomDiagID(
        clang::DiagnosticsEngine::Warning,
        "Invalid assumption in genpybind (please report this): %0");
  case Kind::IgnoringQualifiersOnAliasWarning:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Warning,
                                  "Ignoring qualifiers on alias");
  case Kind::OnlyGlobalScopeAllowedError:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Error,
                                  "'%0' can only be used in global scope");
  case Kind::PreviouslyExposedHereNote:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Note,
                                  "Previously exposed here");
  case Kind::PropertyAlreadyDefinedError:
    return engine.getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "%select{getter|setter}1 already defined for '%0'");
  case Kind::PropertyHasNoGetterError:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Error,
                                  "No getter for the '%0' property");
  case Kind::TrailingParametersError:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Error,
                                  "cannot be followed by other parameters");
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

auto Diagnostics::report(const clang::Decl *decl,
                         Kind kind) -> clang::DiagnosticBuilder {
  clang::DiagnosticsEngine &engine = decl->getASTContext().getDiagnostics();
  return report(decl, getCustomDiagID(engine, kind));
}

auto Diagnostics::report(const clang::Decl *decl,
                         unsigned diag_id) -> clang::DiagnosticBuilder {
  clang::DiagnosticsEngine &engine = decl->getASTContext().getDiagnostics();
  return engine.Report(decl->getLocation(), diag_id) << decl->getSourceRange();
}
