// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/annotations/literal_value.h"
#include "genpybind/annotations/parser.h"

#include <llvm/ADT/StringRef.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <initializer_list>
#include <tuple>
#include <vector>

namespace {

using ::testing::AllOf;
using ::testing::IsEmpty;

using ::genpybind::annotations::LiteralValue;
using Token = ::genpybind::annotations::Parser::Token;
using Tokenizer = ::genpybind::annotations::Parser::Tokenizer;

template <typename FieldMatcher>
// NOLINTNEXTLINE(readability-identifier-naming)
auto TokenKind(const FieldMatcher &matcher) {
  return ::testing::Field("kind", &Token::kind, matcher);
}

template <typename FieldMatcher>
// NOLINTNEXTLINE(readability-identifier-naming)
auto TokenText(const FieldMatcher &matcher) {
  return ::testing::Field("text", &Token::text, matcher);
}

template <typename FieldMatcher>
// NOLINTNEXTLINE(readability-identifier-naming)
auto TokenValue(const FieldMatcher &matcher) {
  return ::testing::Field("value", &Token::value, matcher);
}

TEST(AnnotationsTokenizer, AdvancesPositionWhenConsumingTokens) {
  Tokenizer tokenizer("uiae(xyz)");
  EXPECT_EQ("(xyz)", tokenizer.remaining());
  tokenizer.consumeToken();
  EXPECT_EQ("xyz)", tokenizer.remaining());
}

TEST(AnnotationsTokenizer, YieldsConsecutiveTokensWithLiteralValues) {
  Tokenizer tokenizer(
      R"(visible, keep_alive(1, 2), expose_as(_someId0_), hide_base("::Base"))");
  const std::vector<std::tuple<llvm::StringRef, Token::Kind, LiteralValue>>
      expected_tokens{
          {"visible", Token::Kind::Identifier, {}},
          {",", Token::Kind::Comma, {}},
          {"keep_alive", Token::Kind::Identifier, {}},
          {"(", Token::Kind::OpeningParen, {}},
          {"1", Token::Kind::Literal, LiteralValue::createUnsigned(1)},
          {",", Token::Kind::Comma, {}},
          {"2", Token::Kind::Literal, LiteralValue::createUnsigned(2)},
          {")", Token::Kind::ClosingParen, {}},
          {",", Token::Kind::Comma, {}},
          {"expose_as", Token::Kind::Identifier, {}},
          {"(", Token::Kind::OpeningParen, {}},
          {"_someId0_", Token::Kind::Identifier, {}},
          {")", Token::Kind::ClosingParen, {}},
          {",", Token::Kind::Comma, {}},
          {"hide_base", Token::Kind::Identifier, {}},
          {"(", Token::Kind::OpeningParen, {}},
          {R"("::Base")", Token::Kind::Literal,
           LiteralValue::createString("::Base")},
          {")", Token::Kind::ClosingParen, {}},
          {"", Token::Kind::Eof, {}},
      };

  for (const auto &tup : expected_tokens) {
    EXPECT_THAT(tokenizer.consumeToken(),
                AllOf(TokenText(std::get<0>(tup)), TokenKind(std::get<1>(tup)),
                      TokenValue(std::get<2>(tup))));
  }
  EXPECT_THAT(tokenizer.remaining(), IsEmpty());
}

TEST(AnnotationsTokenizer, CanPeekAtCurrentTokenKindWithoutConsuming) {
  Tokenizer tokenizer("uiae(xyz)");
  EXPECT_EQ(Token::Kind::Identifier, tokenizer.tokenKind());
  EXPECT_EQ("(xyz)", tokenizer.remaining());
  EXPECT_THAT(tokenizer.consumeToken(),
              AllOf(TokenKind(Token::Kind::Identifier), TokenText("uiae")));

  EXPECT_EQ(Token::Kind::OpeningParen, tokenizer.tokenKind());
  EXPECT_EQ("xyz)", tokenizer.remaining());
  EXPECT_THAT(tokenizer.consumeToken(),
              AllOf(TokenKind(Token::Kind::OpeningParen), TokenText("(")));
}

TEST(AnnotationsTokenizer, GivenEmptyStringReturnsEof) {
  Tokenizer tokenizer("");
  const auto consume = [&tokenizer] {
    EXPECT_THAT(tokenizer.consumeToken(),
                AllOf(TokenKind(Token::Kind::Eof), TokenText(""),
                      TokenText(IsEmpty())));
  };
  consume();
  consume(); // EOF is returned indefinitely
}

TEST(AnnotationsTokenizer, IgnoresHorizontalWhitespace) {
  Tokenizer tokenizer("  \t\f\v  uiae");
  EXPECT_THAT(tokenizer.consumeToken(),
              AllOf(TokenKind(Token::Kind::Identifier), TokenText("uiae")));
}

TEST(AnnotationsTokenizer, RejectsVerticalWhitespace) {
  for (llvm::StringRef text : {"\n uiae", "\r uiae"}) {
    Tokenizer tokenizer(text);
    EXPECT_THAT(tokenizer.remaining(), text.drop_front(1));
    EXPECT_THAT(tokenizer.consumeToken(), AllOf(TokenKind(Token::Kind::Invalid),
                                                TokenText(text.take_front(1))));
  }
}

TEST(AnnotationsTokenizer, AllowsEscapesInStringLiterals) {
  for (llvm::StringRef text : {R"("ui\"ae"tail)", R"("uiae\\"tail)"}) {
    Tokenizer tokenizer(text);
    EXPECT_THAT(tokenizer.remaining(), "tail");
    EXPECT_THAT(tokenizer.consumeToken(), AllOf(TokenKind(Token::Kind::Literal),
                                                TokenText(text.take_front(8))));
  }
}

TEST(AnnotationsTokenizer, RejectsUnclosedStringLiterals) {
  for (llvm::StringRef text : {R"("uiae)", R"("uiae\"nrtd\")"}) {
    Tokenizer tokenizer(text);
    EXPECT_THAT(tokenizer.remaining(), "");
    EXPECT_THAT(tokenizer.consumeToken(),
                AllOf(TokenKind(Token::Kind::Invalid), TokenText(text)));
  }
}

} // namespace
