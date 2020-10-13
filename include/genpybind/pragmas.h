#pragma once

#include <clang/Lex/Pragma.h>

#include <vector>

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
