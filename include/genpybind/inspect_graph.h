#pragma once

#include "genpybind/decl_context_graph.h"
#include "genpybind/annotated_decl.h"

#include <llvm/ADT/Twine.h>

namespace genpybind {

void viewGraph(const DeclContextGraph *graph,
               const AnnotationStorage &annotations, const llvm::Twine &name,
               const llvm::Twine &title = "");

} // namespace genpybind
