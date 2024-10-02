// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "genpybind/decl_context_graph_processing.h"

#include <llvm/ADT/Twine.h>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace genpybind {
class AnnotationStorage;
class DeclContextGraph;

void viewGraph(const DeclContextGraph *graph,
               const AnnotationStorage &annotations, const llvm::Twine &name,
               const llvm::Twine &title = "");

void printGraph(llvm::raw_ostream &os, const DeclContextGraph *graph,
                const EffectiveVisibilityMap &visibilities,
                const AnnotationStorage &annotations,
                const llvm::Twine &title = "");

} // namespace genpybind
