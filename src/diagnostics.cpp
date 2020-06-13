#include "genpybind/diagnostics.h"

#include <clang/AST/ASTContext.h>

using namespace genpybind;

static unsigned getCustomDiagID(clang::DiagnosticsEngine &engine,
                                Diagnostics::Kind kind) {
  using Kind = Diagnostics::Kind;
  switch (kind) {
  case Kind::IgnoringQualifiersOnAliasWarning:
    return engine.getCustomDiagID(clang::DiagnosticsEngine::Warning,
                                  "Ignoring qualifiers on alias");
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
