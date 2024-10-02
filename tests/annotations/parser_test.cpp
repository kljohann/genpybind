// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/annotations/parser.h"

#include "expected.h"

#include <llvm/Testing/Support/Error.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <initializer_list>
#include <tuple>

namespace {

using ::genpybind::TheValue;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::SizeIs;

using ::genpybind::annotations::Annotation;
using ::genpybind::annotations::AnnotationKind;
using ::genpybind::annotations::LiteralValue;
using ::genpybind::annotations::Parser;

template <typename PropertyMatcher>
// NOLINTNEXTLINE(readability-identifier-naming)
auto ErrorKind(const PropertyMatcher &matcher) {
  return ::testing::Property("kind", &Parser::Error::getKind, matcher);
}

template <typename PropertyMatcher>
// NOLINTNEXTLINE(readability-identifier-naming)
auto ErrorToken(const PropertyMatcher &matcher) {
  return ::testing::Property("token", &Parser::Error::getToken, matcher);
}

template <typename ArgumentsMatcher>
// NOLINTNEXTLINE(readability-identifier-naming)
auto AnnotationWhere(AnnotationKind kind, const ArgumentsMatcher &matcher) {
  return AllOf(
      ::testing::Property("kind", &Annotation::getKind, kind),
      ::testing::Property("arguments", &Annotation::getArguments, matcher));
}

TEST(AnnotationsParser, GivenEmptyStringReturnsEmptySequence) {
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations(""), TheValue(IsEmpty()));
}

TEST(AnnotationsParser, RejectsIsolatedComma) {
  EXPECT_THAT_ERROR(
      Parser::parseAnnotations(",").takeError(),
      llvm::Failed<Parser::Error>(AllOf(
          ErrorKind(Parser::Error::Kind::InvalidToken), ErrorToken(","))));
}

TEST(AnnotationsParser, RejectsAnnotationsWithInvalidName) {
  EXPECT_THAT_ERROR(Parser::parseAnnotations("uiae").takeError(),
                    llvm::Failed<Parser::Error>(
                        AllOf(ErrorKind(Parser::Error::Kind::InvalidAnnotation),
                              ErrorToken("uiae"))));

  EXPECT_THAT_ERROR(Parser::parseAnnotations("visible, uiae()").takeError(),
                    llvm::Failed<Parser::Error>(
                        AllOf(ErrorKind(Parser::Error::Kind::InvalidAnnotation),
                              ErrorToken("uiae"))));
}

TEST(AnnotationsParser, RecognizesAllAnnotationKinds) {
#define ANNOTATION_KIND(Enum, Spelling)                                        \
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations(#Spelling "()"),               \
                       TheValue(ElementsAre(AnnotationWhere(                   \
                           AnnotationKind::Enum, IsEmpty()))));
#include "genpybind/annotations/annotations.def"
}

TEST(AnnotationsParser, RejectsInvalidAnnotations) {
  struct Example {
    llvm::StringRef text;
    llvm::StringRef token;
  };
  for (auto example : std::initializer_list<Example>{
           {".", {}},
           {",", {}},
           {"(", {}},
           {")", {}},
           {R"("str")", {}},
           {"1", {}},
           {R"(expose_as("string_not_closed))", R"("string_not_closed))"},
           {R"(expose_as(string_not_opened"))", R"x("))x"},
           {"expose_as 123", "123"},
           {"expose_as true", "true"},
           {R"(expose_as "uiae")", R"("uiae")"},
           {"\n", {}},
           {"visible,\n expose_as(__int__)\r\n", "\n"},
           {"visible.", "."},
           {"visible)", ")"},
       }) {
    llvm::StringRef token =
        example.token.empty() ? example.text : example.token;
    EXPECT_THAT_ERROR(
        Parser::parseAnnotations(example.text).takeError(),
        llvm::Failed<Parser::Error>(AllOf(
            ErrorKind(Parser::Error::Kind::InvalidToken), ErrorToken(token))));
  }
  for (auto example : std::initializer_list<Example>{
           {R"(expose_as(nested(parens)))", "("},
           {R"(expose_as("no_trailing_paren")", ""},
           {"visible(default 123)", "123"},
       }) {
    EXPECT_THAT_ERROR(Parser::parseAnnotations(example.text).takeError(),
                      llvm::Failed<Parser::Error>(AllOf(
                          ErrorKind(Parser::Error::Kind::MissingClosingParen),
                          ErrorToken(example.token))));
  }
}

TEST(AnnotationsParser, AcceptsTrailingComma) {
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations("visible,"),
                       TheValue(SizeIs(1)));
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations("visible, expose_as(__int__),"),
                       TheValue(SizeIs(2)));
}

TEST(AnnotationsParser, AcceptsTrailingCommaInArguments) {
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations("expose_as(__int__,)"),
                       TheValue(ElementsAre(AnnotationWhere(
                           AnnotationKind::ExposeAs, SizeIs(1)))));
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations("keep_alive(this, child,)"),
                       TheValue(ElementsAre(AnnotationWhere(
                           AnnotationKind::KeepAlive, SizeIs(2)))));
}

TEST(AnnotationsParser, IgnoresHorizontalWhitespace) {
  EXPECT_THAT_EXPECTED(
      Parser::parseAnnotations(
          "  \f visible \t, \v keep_alive (\" ui \t ae \"\t, nrtd )  "),
      TheValue(ElementsAre(
          AnnotationWhere(AnnotationKind::Visible, IsEmpty()),
          AnnotationWhere(AnnotationKind::KeepAlive,
                          ElementsAre(LiteralValue::createString(" ui \t ae "),
                                      LiteralValue::createString("nrtd"))))));
}

TEST(AnnotationsParser, AcceptsAnnotationWithoutArguments) {
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations("visible"),
                       TheValue(ElementsAre(AnnotationWhere(
                           AnnotationKind::Visible, IsEmpty()))));
}

TEST(AnnotationsParser, AcceptsAnnotationWithIdentifierAsArgument) {
  EXPECT_THAT_EXPECTED(
      Parser::parseAnnotations(R"(expose_as(visible))"),
      TheValue(ElementsAre(AnnotationWhere(
          AnnotationKind::ExposeAs,
          ElementsAre(LiteralValue::createString("visible"))))));
}

TEST(AnnotationsParser, AcceptsAnnotationWithStringArgument) {
  EXPECT_THAT_EXPECTED(
      Parser::parseAnnotations(R"(expose_as("__int__"))"),
      TheValue(ElementsAre(AnnotationWhere(
          AnnotationKind::ExposeAs,
          ElementsAre(LiteralValue::createString("__int__"))))));
}

TEST(AnnotationsParser, AcceptsAnnotationWithUnsignedArguments) {
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations("keep_alive(1, 2)"),
                       TheValue(ElementsAre(AnnotationWhere(
                           AnnotationKind::KeepAlive,
                           ElementsAre(LiteralValue::createUnsigned(1),
                                       LiteralValue::createUnsigned(2))))));
}

TEST(AnnotationsParser, AcceptsAnnotationWithBooleanArgument) {
  EXPECT_THAT_EXPECTED(
      Parser::parseAnnotations("visible(false), hidden(true)"),
      TheValue(ElementsAre(
          AnnotationWhere(AnnotationKind::Visible,
                          ElementsAre(LiteralValue::createBoolean(false))),
          AnnotationWhere(AnnotationKind::Hidden,
                          ElementsAre(LiteralValue::createBoolean(true))))));
}

TEST(AnnotationsParser, AcceptsAnnotationWithDefaultArgument) {
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations("visible(default)"),
                       TheValue(ElementsAre(AnnotationWhere(
                           AnnotationKind::Visible,
                           ElementsAre(LiteralValue::createDefault())))));
}

TEST(AnnotationsParser, AcceptsMultipleAnnotations) {
  EXPECT_THAT_EXPECTED(
      Parser::parseAnnotations(
          R"(visible, keep_alive(1, 2), expose_as(_someId0_), hide_base("::Base"))"),
      TheValue(ElementsAre(
          AnnotationWhere(AnnotationKind::Visible, IsEmpty()),
          AnnotationWhere(AnnotationKind::KeepAlive,
                          ElementsAre(LiteralValue::createUnsigned(1),
                                      LiteralValue::createUnsigned(2))),
          AnnotationWhere(AnnotationKind::ExposeAs,
                          ElementsAre(LiteralValue::createString("_someId0_"))),
          AnnotationWhere(AnnotationKind::HideBase,
                          ElementsAre(LiteralValue::createString("::Base"))))));
}

} // namespace
