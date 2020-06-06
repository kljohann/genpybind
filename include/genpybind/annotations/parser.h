#pragma once

#include <llvm/ADT/StringRef.h>

namespace genpybind {
namespace annotations {

class Parser {
public:
  struct Token;
  class Tokenizer;
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
};

class Parser::Tokenizer {
  llvm::StringRef text;
  Token next_token;

public:
  Tokenizer(llvm::StringRef text) : text(text) { next_token = tokenize(); }

  llvm::StringRef remaining() const { return text; }

  const Token &peekToken() const { return next_token; }

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

}  // namespace annotations
} // namespace genpybind
