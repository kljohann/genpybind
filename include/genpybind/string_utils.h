// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <llvm/ADT/StringRef.h>

namespace llvm {
template <typename T> class SmallVectorImpl;
} // namespace llvm

namespace genpybind {

/// Return whether `name` is a valid identifier.
bool isValidIdentifier(llvm::StringRef name);

/// Replace consecutive runs of non-identifier characters in `name` by
/// single underscores.
void makeValidIdentifier(llvm::SmallVectorImpl<char> &name);

} // namespace genpybind
