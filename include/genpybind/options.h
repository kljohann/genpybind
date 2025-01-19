// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

namespace llvm::cl {
class OptionCategory;
} // namespace llvm::cl

namespace genpybind {

enum class Experiment {
  Aggregates,
};
bool isEnabled(Experiment experiment);

llvm::cl::OptionCategory &getGenpybindCategory();

} // namespace genpybind
