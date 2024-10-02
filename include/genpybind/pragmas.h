// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <clang/Lex/Pragma.h>

#include <string>
#include <vector>

namespace clang {
class Preprocessor;
class Token;
} // namespace clang

namespace genpybind {

class PragmaGenpybindHandler : public clang::PragmaHandler {
  // TODO: Set?
  std::vector<std::string> includes;

public:
  PragmaGenpybindHandler();

  void HandlePragma(clang::Preprocessor &preproc,
                    clang::PragmaIntroducer introducer,
                    clang::Token &handler_name_token) override;

  const std::vector<std::string> &getIncludes() const { return includes; }
};

} // namespace genpybind
