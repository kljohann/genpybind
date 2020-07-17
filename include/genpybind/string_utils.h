#pragma once

namespace llvm {
template <typename T> class SmallVectorImpl;
} // namespace llvm

namespace genpybind {

/// Replace consecutive runs of non-identifier characters in `name` by
/// single underscores.
void makeValidIdentifier(llvm::SmallVectorImpl<char> &name);

} // namespace genpybind
