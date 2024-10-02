// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/pragmas.h"

#include "clang/Basic/TokenKinds.h"

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticLex.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Pragma.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/StringRef.h>

#include <cassert>
#include <utility>

using namespace genpybind;

PragmaGenpybindHandler::PragmaGenpybindHandler() : PragmaHandler("genpybind") {}

void PragmaGenpybindHandler::HandlePragma(clang::Preprocessor &preproc,
                                          clang::PragmaIntroducer introducer,
                                          clang::Token &handler_name_token) {
  // In order to capture the directive but also apply it, pretend that this was
  // a normal directive.  Ideally the tokens would be lexed here and appropriate
  // replacement tokens pushed back using `EnterTokenStream`.
  // However `HandleDirective` only works when lexing directly from source code.
  clang::Token hash_token{};
  hash_token.startToken();
  hash_token.setLocation(introducer.Loc);
  hash_token.setFlag(clang::Token::StartOfLine);
  hash_token.setKind(clang::tok::hash);

  clang::DiagnosticErrorTrap trap{preproc.getDiagnostics()};
  preproc.HandleDirective(hash_token);
  if (trap.hasErrorOccurred())
    return;

  // Now go back and try to figure out what the directive was.
  const clang::SourceManager &source_manager = preproc.getSourceManager();
  std::pair<clang::FileID, unsigned> begin_loc =
      source_manager.getDecomposedLoc(handler_name_token.getLocation());
  bool buffer_invalid = false;
  llvm::StringRef buffer =
      source_manager.getBufferData(begin_loc.first, &buffer_invalid);
  assert(!buffer_invalid);
  if (buffer_invalid)
    return;

  clang::Lexer lexer(source_manager.getLocForStartOfFile(begin_loc.first),
                     preproc.getLangOpts(), buffer.begin(),
                     buffer.data() + begin_loc.second, buffer.end());
  lexer.setParsingPreprocessorDirective(true);
  assert(!lexer.isKeepWhitespaceMode() || !lexer.inKeepCommentMode());

  clang::Token token{};
  clang::Token directive_token{};
  if (lexer.LexFromRawLexer(token) || lexer.LexFromRawLexer(directive_token) ||
      !directive_token.isAnyIdentifier()) {
    preproc.Diag(handler_name_token.getEndLoc(),
                 clang::diag::err_pp_invalid_directive)
        << 0;
    return;
  }

  if (directive_token.getRawIdentifier() == "include") {
    lexer.LexIncludeFilename(token);
    if (!token.isOneOf(clang::tok::string_literal, clang::tok::header_name)) {
      preproc.Diag(token.getLocation(), clang::diag::err_pp_expects_filename);
      return;
    }
    std::string argument = preproc.getSpelling(token);
    if (argument.empty()) {
      preproc.Diag(token.getLocation(), clang::diag::err_pp_empty_filename);
      return;
    }
    includes.push_back(argument);
  } else {
    preproc.Diag(directive_token.getLocation(),
                 clang::diag::err_pp_invalid_directive)
        << 0;
    return;
  }

  if (lexer.LexFromRawLexer(token))
    return;
  if (!token.isOneOf(clang::tok::eof, clang::tok::eod)) {
    preproc.Diag(token.getLocation(), clang::diag::err_pp_expected_eol);
    return;
  }
}
