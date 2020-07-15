#pragma once

#include <llvm/ADT/SmallVector.h>

namespace genpybind {

/// Replace consecutive runs of non-identifier characters in `name` by
/// single underscores.
void makeValidIdentifier(llvm::SmallVectorImpl<char> &name);

} // namespace genpybind
