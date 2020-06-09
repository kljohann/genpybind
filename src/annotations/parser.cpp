#include "genpybind/annotations/parser.h"

#include <clang/Basic/CharInfo.h>
#include <llvm/ADT/Sequence.h>
#include <llvm/ADT/StringSwitch.h>

#include <cassert>
#include <llvm/Support/Error.h>

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

auto Parser::parseAnnotations(llvm::StringRef text)
    -> llvm::Expected<Annotations> {
  Annotations annotations;
  if (llvm::Error error = parseAnnotations(text, annotations))
    return {std::move(error)};
  return annotations;
}

auto Parser::parseAnnotations(llvm::StringRef text, Annotations &annotations)
    -> llvm::Error {
  Tokenizer tokenizer(text);
  return Parser(&tokenizer).parseAnnotations(annotations);
}

Parser::Parser(Tokenizer *tokenizer) : tokenizer(tokenizer) {
  assert(tokenizer != nullptr);
}

auto Parser::parseAnnotations(Annotations &annotations) -> llvm::Error {
  while (tokenizer->tokenKind() == Token::Kind::Identifier) {
    auto kind = parseAnnotationKind();
    if (!kind)
      return kind.takeError();

    auto arguments = parseAnnotationArguments();
    if (!arguments)
      return arguments.takeError();

    annotations.emplace_back(*kind, std::move(*arguments));
    if (!skipComma())
      break;
  }

  if (tokenizer->tokenKind() != Token::Kind::Eof)
    return llvm::make_error<Error>(Error::Kind::InvalidToken,
                                   tokenizer->consumeToken().text);

  return llvm::Error::success();
}

auto Parser::parseAnnotationKind() -> llvm::Expected<AnnotationKind> {
  Token token = tokenizer->consumeToken();
  assert(token.kind == Token::Kind::Identifier);
  return llvm::StringSwitch<llvm::Expected<AnnotationKind>>(token.text)
#define ANNOTATION_KIND(Enum, Spelling) .Case(#Spelling, AnnotationKind::Enum)
#include "genpybind/annotations/annotations.def"
      .Default(
          llvm::make_error<Error>(Error::Kind::InvalidAnnotation, token.text));
}

auto Parser::parseAnnotationArguments()
    -> llvm::Expected<Annotation::Arguments> {
  Annotation::Arguments arguments;

  if (tokenizer->tokenKind() != Token::Kind::OpeningParen) {
    // It's valid to omit the parens if no arguments are passed.
    return arguments;
  }
  tokenizer->consumeToken();

  while (LiteralValue value = parseAnnotationValue()) {
    arguments.push_back(std::move(value));
    if (!skipComma()) {
      break;
    }
  }

  Token token = tokenizer->consumeToken();
  if (token.kind != Token::Kind::ClosingParen) {
    return llvm::make_error<Error>(token.kind == Token::Kind::Invalid
                                       ? Error::Kind::InvalidToken
                                       : Error::Kind::MissingClosingParen,
                                   token.text);
  }
  return arguments;
}

auto Parser::parseAnnotationValue() -> LiteralValue {
  if (tokenizer->tokenKind() == Token::Kind::Literal) {
    Token token = tokenizer->consumeToken();
    return std::move(token.value);
  }
  if (tokenizer->tokenKind() == Token::Kind::Identifier) {
    // Identifiers are implicitly converted to special values or strings when
    // used as arguments.
    Token token = tokenizer->consumeToken();
    return llvm::StringSwitch<LiteralValue>(token.text)
        .Case("true", LiteralValue::createBoolean(true))
        .Case("false", LiteralValue::createBoolean(false))
        .Case("default", LiteralValue::createDefault())
        .Default(LiteralValue::createString(token.text));
  }
  return {};
}

bool Parser::skipComma() {
  if (tokenizer->tokenKind() != Token::Kind::Comma)
    return false;
  tokenizer->consumeToken();
  return true;
}

static unsigned getCustomDiagID(clang::DiagnosticsEngine &diagnostics,
                                Parser::Error::Kind kind) {
  using Kind = Parser::Error::Kind;
  switch (kind) {
  case Kind::InvalidToken:
    return diagnostics.getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "Invalid token in genpybind annotation: %0");
    break;
  case Kind::MissingClosingParen:
    return diagnostics.getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "Invalid token in genpybind annotation while looking for ')': %0");
    break;
  case Kind::InvalidAnnotation:
    return diagnostics.getCustomDiagID(clang::DiagnosticsEngine::Error,
                                       "Invalid genpybind annotation: %0");
    break;
  }
  llvm_unreachable("Unknown parser error.");
}

char Parser::Error::ID;

clang::DiagnosticBuilder
Parser::Error::report(clang::SourceLocation loc,
                      clang::DiagnosticsEngine &diagnostics) const {
  return diagnostics.Report(loc, getCustomDiagID(diagnostics, kind)) << token;
}

} // namespace annotations
} // namespace genpybind
