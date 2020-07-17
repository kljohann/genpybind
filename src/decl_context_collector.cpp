#include "genpybind/decl_context_collector.h"

#include "genpybind/diagnostics.h"

#include <clang/AST/Type.h>

using namespace genpybind;

void DeclContextCollector::warnIfAliasHasQualifiers(
    const clang::TypedefNameDecl *decl) {
  clang::QualType qual_type = decl->getUnderlyingType();
  if (qual_type.hasQualifiers())
    Diagnostics::report(decl,
                        Diagnostics::Kind::IgnoringQualifiersOnAliasWarning);
}
