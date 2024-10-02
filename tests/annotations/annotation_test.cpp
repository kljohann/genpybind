// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/annotations/annotation.h"

#include "genpybind/annotations/literal_value.h"

#include <llvm/Support/raw_ostream.h>

#include <gtest/gtest.h>

namespace {

using namespace ::genpybind::annotations;

TEST(Annotation, CanBePrinted) {
  std::string result;
  llvm::raw_string_ostream stream(result);

  Annotation(AnnotationKind::Visible).print(stream);
  stream << ' ';
  Annotation(AnnotationKind::Visible, {LiteralValue::createDefault()})
      .print(stream);
  stream << ' ';
  Annotation(AnnotationKind::Hidden, {LiteralValue::createBoolean(true)})
      .print(stream);
  stream << ' ';
  Annotation(AnnotationKind::KeepAlive,
             {LiteralValue::createUnsigned(1), LiteralValue::createUnsigned(2)})
      .print(stream);
  stream << ' ';
  Annotation(AnnotationKind::ExposeAs,
             {LiteralValue::createString("_someId0_")})
      .print(stream);
  stream.flush();
  EXPECT_EQ(
      R"(visible() visible(default) hidden(true) keep_alive(1, 2) expose_as("_someId0_"))",
      result);
}

} // namespace
