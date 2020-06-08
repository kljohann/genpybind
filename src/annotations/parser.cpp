#include "genpybind/annotations/parser.h"

#include <clang/Basic/CharInfo.h>
#include <llvm/ADT/Sequence.h>

#include <cassert>

namespace genpybind {
namespace annotations {

auto Parser::Tokenizer::tokenize() -> Token {
  Token result;

  text = text.drop_while(clang::isHorizontalWhitespace);

  if (text.empty()) {
    result.kind = Token::Kind::Eof;
    result.text = "";
    return result;
  }

  const auto tokenizeChar = [&result, &text = text](Token::Kind kind) {
    result.kind = kind;
    result.text = text.take_front(1);
    text = text.drop_front(1);
  };

  switch (text.front()) {
  case '(':
    tokenizeChar(Token::Kind::OpeningParen);
    break;
  case ')':
    tokenizeChar(Token::Kind::ClosingParen);
    break;
  case ',':
    tokenizeChar(Token::Kind::Comma);
    break;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    tokenizeNumberLiteral(result);
    break;
  case '"':
    tokenizeStringLiteral(result);
    break;
  default:
    if (clang::isIdentifierHead(text.front())) {
      tokenizeIdentifier(result);
    } else {
      tokenizeChar(Token::Kind::Invalid);
    }
    break;
  }

  return result;
}

void Parser::Tokenizer::tokenizeNumberLiteral(Token &result) {
  assert(clang::isDigit(text.front()));

  result.kind = Token::Kind::Literal;
  // Only unsigned integers (without sign char) are supported.
  result.text = text.take_while(clang::isDigit);
  text = text.drop_front(result.text.size());

  unsigned value;
  if (!result.text.getAsInteger(10, value)) {
    result.value.setUnsigned(value);
    return;
  }
  result.kind = Token::Kind::Invalid;
}

void Parser::Tokenizer::tokenizeStringLiteral(Token &result) {
  assert(text.front() == '"');
  bool escape_next_char = false;
  for (size_t pos : llvm::seq<size_t>(1, text.size())) {
    if (escape_next_char || text[pos] == '\\') {
      escape_next_char = !escape_next_char;
      continue;
    }
    if (text[pos] == '"') {
      result.kind = Token::Kind::Literal;
      result.text = text.take_front(pos + 1);
      result.value.setString(text.substr(1, pos - 1));
      text = text.drop_front(pos + 1);
      return;
    }
  }
  result.text = text;
  result.kind = Token::Kind::Invalid;
  text = text.drop_front(text.size());
}

void Parser::Tokenizer::tokenizeIdentifier(Token &result) {
  assert(clang::isIdentifierHead(text.front()));

  result.kind = Token::Kind::Identifier;
  result.text = text.take_while(
      [](unsigned char c) { return clang::isIdentifierBody(c); });
  assert(result.text.size() >= 1);
  text = text.drop_front(result.text.size());
}

} // namespace annotations
} // namespace genpybind
