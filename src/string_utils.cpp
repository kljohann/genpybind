// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/string_utils.h"

#include <clang/Basic/CharInfo.h>
#include <llvm/ADT/SmallVector.h>

#include <algorithm>

bool genpybind::isValidIdentifier(llvm::StringRef name) {
  return clang::isValidAsciiIdentifier(name);
}

void genpybind::makeValidIdentifier(llvm::SmallVectorImpl<char> &name) {
  char *const end = name.end();
  char *last_valid = name.begin();
  char *output = name.begin();

  const auto is_valid_head = [](unsigned char c) {
    return clang::isAsciiIdentifierStart(c);
  };
  const auto is_valid_body = [](unsigned char c) {
    return clang::isAsciiIdentifierContinue(c);
  };

  // Skip invalid characters at the start.
  char *input = std::find_if(last_valid, end, is_valid_head);

  for (; input != end; input = std::find_if(input, end, is_valid_body)) {
    // Insert underscore if any characters have been skipped.
    if (input != last_valid)
      *output++ = '_';
    *output++ = *input++;
    last_valid = input;
  }

  // Insert underscore if there are trailing invalid characters.
  if (last_valid != end)
    *output++ = '_';
  name.erase(output, end);
}
