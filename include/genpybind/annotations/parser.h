// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "genpybind/annotations/annotation.h"
#include "genpybind/annotations/literal_value.h"

#include <clang/Basic/Diagnostic.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>

#include <system_error>
#include <vector>

namespace clang {
class SourceLocation;
} // namespace clang

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace genpybind {
namespace annotations {

class Parser {
public:
  class Error;
  class Tokenizer;
  struct Token;

  using Annotations = std::vector<Annotation>;

  static llvm::Error parseAnnotations(llvm::StringRef text,
                                      Annotations &annotations);
  static llvm::Expected<Annotations> parseAnnotations(llvm::StringRef text);

private:
  Parser(Tokenizer *tokenizer);
  llvm::Error parseAnnotations(Annotations &annotations);
  llvm::Expected<AnnotationKind> parseAnnotationKind();
  llvm::Expected<Annotation::Arguments> parseAnnotationArguments();
  LiteralValue parseAnnotationValue();
  bool skipComma();

  Tokenizer *tokenizer;
};

class Parser::Error : public llvm::ErrorInfo<Parser::Error> {
public:
  enum class Kind {
    InvalidToken,
    MissingClosingParen,
    InvalidAnnotation,
  };

  Error(Kind kind, llvm::StringRef token) : kind(kind), token(token) {}

  Kind getKind() const { return kind; }
  llvm::StringRef getToken() const { return token; }

  clang::DiagnosticBuilder report(clang::SourceLocation loc,
                                  clang::DiagnosticsEngine &diagnostics) const;

  void log(llvm::raw_ostream &os) const override;

  static char ID;

private:
  Kind kind;
  llvm::StringRef token;

  std::error_code convertToErrorCode() const override {
    return llvm::inconvertibleErrorCode();
  }
};

struct Parser::Token {
  enum class Kind {
    Eof,
    Identifier,
    Literal,
    OpeningParen,
    ClosingParen,
    Comma,
    Invalid,
  };

  Kind kind = Kind::Eof;
  llvm::StringRef text;
  LiteralValue value;
};

class Parser::Tokenizer {
  llvm::StringRef text;
  Token next_token;

public:
  Tokenizer(llvm::StringRef text) : text(text) { next_token = tokenize(); }

  llvm::StringRef remaining() const { return text; }
  Token::Kind tokenKind() const { return next_token.kind; }
  Token consumeToken() {
    Token token = next_token;
    next_token = tokenize();
    return token;
  }

private:
  Token tokenize();
  void tokenizeNumberLiteral(Token &result);
  void tokenizeStringLiteral(Token &result);
  void tokenizeIdentifier(Token &result);
};

} // namespace annotations
} // namespace genpybind
