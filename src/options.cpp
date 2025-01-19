// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/options.h"

#include <llvm/Support/CommandLine.h>

using namespace genpybind;

namespace {

llvm::cl::opt<bool>
    g_all_experiments("all-experiments", llvm::cl::cat(getGenpybindCategory()),
                      llvm::cl::desc("Enable all experimental features"),
                      llvm::cl::init(false), llvm::cl::Hidden);

llvm::cl::bits<Experiment> g_experiments(
    "experiment", llvm::cl::cat(getGenpybindCategory()),
    llvm::cl::desc("Enable experimental features"),
    llvm::cl::values(clEnumValN(Experiment::Aggregates, "aggregates",
                                "Emit constructors for aggregates")),
    llvm::cl::Hidden);

} // namespace

bool genpybind::isEnabled(Experiment experiment) {
  return g_all_experiments || g_experiments.isSet(experiment);
}

llvm::cl::OptionCategory &genpybind::getGenpybindCategory() {
  static llvm::cl::OptionCategory category{"Genpybind options"};
  return category;
}
