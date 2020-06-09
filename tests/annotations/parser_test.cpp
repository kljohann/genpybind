#include "genpybind/annotations/parser.h"

#include <llvm/Testing/Support/Error.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// HACK: This is missing from libLLVM.so on Fedora.
inline llvm::detail::ErrorHolder llvm::detail::TakeError(llvm::Error Err) {
  std::vector<std::shared_ptr<llvm::ErrorInfoBase>> Infos;
  llvm::handleAllErrors(std::move(Err),
                        [&Infos](std::unique_ptr<ErrorInfoBase> Info) {
                          Infos.emplace_back(std::move(Info));
                        });
  return {std::move(Infos)};
}

namespace {

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
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations(""), llvm::HasValue(IsEmpty()));
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
                       llvm::HasValue(ElementsAre(AnnotationWhere(             \
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
                       llvm::HasValue(SizeIs(1)));
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations("visible, expose_as(__int__),"),
                       llvm::HasValue(SizeIs(2)));
}

TEST(AnnotationsParser, AcceptsTrailingCommaInArguments) {
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations("expose_as(__int__,)"),
                       llvm::HasValue(ElementsAre(AnnotationWhere(
                           AnnotationKind::ExposeAs, SizeIs(1)))));
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations("keep_alive(this, child,)"),
                       llvm::HasValue(ElementsAre(AnnotationWhere(
                           AnnotationKind::KeepAlive, SizeIs(2)))));
}

TEST(AnnotationsParser, IgnoresHorizontalWhitespace) {
  EXPECT_THAT_EXPECTED(
      Parser::parseAnnotations(
          "  \f visible \t, \v keep_alive (\" ui \t ae \"\t, nrtd )  "),
      llvm::HasValue(ElementsAre(
          AnnotationWhere(AnnotationKind::Visible, IsEmpty()),
          AnnotationWhere(AnnotationKind::KeepAlive,
                          ElementsAre(LiteralValue::createString(" ui \t ae "),
                                      LiteralValue::createString("nrtd"))))));
}

TEST(AnnotationsParser, AcceptsAnnotationWithoutArguments) {
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations("visible"),
                       llvm::HasValue(ElementsAre(AnnotationWhere(
                           AnnotationKind::Visible, IsEmpty()))));
}

TEST(AnnotationsParser, AcceptsAnnotationWithIdentifierAsArgument) {
  EXPECT_THAT_EXPECTED(
      Parser::parseAnnotations(R"(expose_as(visible))"),
      llvm::HasValue(ElementsAre(AnnotationWhere(
          AnnotationKind::ExposeAs,
          ElementsAre(LiteralValue::createString("visible"))))));
}

TEST(AnnotationsParser, AcceptsAnnotationWithStringArgument) {
  EXPECT_THAT_EXPECTED(
      Parser::parseAnnotations(R"(expose_as("__int__"))"),
      llvm::HasValue(ElementsAre(AnnotationWhere(
          AnnotationKind::ExposeAs,
          ElementsAre(LiteralValue::createString("__int__"))))));
}

TEST(AnnotationsParser, AcceptsAnnotationWithUnsignedArguments) {
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations("keep_alive(1, 2)"),
                       llvm::HasValue(ElementsAre(AnnotationWhere(
                           AnnotationKind::KeepAlive,
                           ElementsAre(LiteralValue::createUnsigned(1),
                                       LiteralValue::createUnsigned(2))))));
}

TEST(AnnotationsParser, AcceptsAnnotationWithBooleanArgument) {
  EXPECT_THAT_EXPECTED(
      Parser::parseAnnotations("visible(false), hidden(true)"),
      llvm::HasValue(ElementsAre(
          AnnotationWhere(AnnotationKind::Visible,
                          ElementsAre(LiteralValue::createBoolean(false))),
          AnnotationWhere(AnnotationKind::Hidden,
                          ElementsAre(LiteralValue::createBoolean(true))))));
}

TEST(AnnotationsParser, AcceptsAnnotationWithDefaultArgument) {
  EXPECT_THAT_EXPECTED(Parser::parseAnnotations("visible(default)"),
                       llvm::HasValue(ElementsAre(AnnotationWhere(
                           AnnotationKind::Visible,
                           ElementsAre(LiteralValue::createDefault())))));
}

TEST(AnnotationsParser, AcceptsMultipleAnnotations) {
  EXPECT_THAT_EXPECTED(
      Parser::parseAnnotations(
          R"(visible, keep_alive(1, 2), expose_as(_someId0_), hide_base("::Base"))"),
      llvm::HasValue(ElementsAre(
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
