#pragma once

#include <clang/AST/Decl.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>

namespace genpybind {

class Diagnostics {
  clang::DiagnosticsEngine &engine;

public:
  enum class Kind {
    IgnoringQualifiersOnAliasWarning,
  };

  Diagnostics(clang::DiagnosticsEngine &engine) : engine(engine) {}

  static clang::DiagnosticBuilder report(const clang::Decl *decl, Kind kind);
  clang::DiagnosticBuilder report(clang::SourceLocation loc, Kind kind);
};

} // namespace genpybind