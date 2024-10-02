// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/annotations/literal_value.h"

#include <llvm/Support/raw_ostream.h>

#include <gtest/gtest.h>

namespace {

using ::genpybind::annotations::LiteralValue;

TEST(LiteralValue, ByDefaultHasNoValue) {
  LiteralValue value;
  EXPECT_FALSE(value);
  EXPECT_FALSE(value.hasValue());
  EXPECT_TRUE(value.isNothing());

  EXPECT_FALSE(value.isString());
  EXPECT_FALSE(value.isUnsigned());
  EXPECT_FALSE(value.isBoolean());
  EXPECT_FALSE(value.isDefault());

  LiteralValue copy(value);
  EXPECT_TRUE(value.isNothing());
  EXPECT_EQ(value, copy);
  EXPECT_NE(LiteralValue::createDefault(), copy);
}

TEST(LiteralValue, CanRepresentStrings) {
  auto value = LiteralValue::createString("uiae");
  EXPECT_TRUE(value.isString());
  EXPECT_TRUE(value);
  EXPECT_TRUE(value.hasValue());
  EXPECT_FALSE(value.isNothing());
  EXPECT_EQ("uiae", value.getString());

  LiteralValue copy(value);
  EXPECT_TRUE(copy.isString());
  EXPECT_EQ("uiae", copy.getString());
  EXPECT_EQ(value, copy);
  EXPECT_NE(LiteralValue(), copy);

  value.setString("asdf");
  EXPECT_EQ("asdf", value.getString());
  EXPECT_NE(value, copy);
}

TEST(LiteralValue, CanRepresentUnsignedIntegers) {
  auto value = LiteralValue::createUnsigned(123u);
  EXPECT_TRUE(value.isUnsigned());
  EXPECT_TRUE(value);
  EXPECT_TRUE(value.hasValue());
  EXPECT_FALSE(value.isNothing());
  EXPECT_EQ(123u, value.getUnsigned());

  LiteralValue copy(value);
  EXPECT_TRUE(value.isUnsigned());
  EXPECT_EQ(123u, value.getUnsigned());
  EXPECT_EQ(value, copy);
  EXPECT_NE(LiteralValue(), copy);

  value.setUnsigned(321u);
  EXPECT_EQ(321u, value.getUnsigned());
  EXPECT_NE(value, copy);
}

TEST(LiteralValue, CanRepresentBooleans) {
  auto value = LiteralValue::createBoolean(false);
  EXPECT_TRUE(value.isBoolean());
  EXPECT_TRUE(value);
  EXPECT_TRUE(value.hasValue());
  EXPECT_FALSE(value.isNothing());
  EXPECT_EQ(false, value.getBoolean());

  LiteralValue copy(value);
  EXPECT_TRUE(value.isBoolean());
  EXPECT_EQ(false, value.getBoolean());
  EXPECT_EQ(value, copy);
  EXPECT_NE(LiteralValue(), copy);

  value.setBoolean(true);
  EXPECT_EQ(true, value.getBoolean());
  EXPECT_NE(value, copy);
}

TEST(LiteralValue, CanRepresentDefaults) {
  auto value = LiteralValue::createDefault();
  EXPECT_TRUE(value.isDefault());
  EXPECT_TRUE(value);
  EXPECT_TRUE(value.hasValue());
  EXPECT_FALSE(value.isNothing());

  LiteralValue copy(value);
  EXPECT_TRUE(value.isDefault());
  EXPECT_EQ(value, copy);
  EXPECT_NE(LiteralValue(), copy);
}

TEST(LiteralValue, CanChangeType) {
  auto value = LiteralValue::createDefault();
  EXPECT_TRUE(value.isDefault());
  value.setBoolean(false);
  EXPECT_FALSE(value.isDefault());
  EXPECT_TRUE(value.isBoolean());
  EXPECT_EQ(false, value.getBoolean());
  value.setUnsigned(123u);
  EXPECT_FALSE(value.isBoolean());
  EXPECT_TRUE(value.isUnsigned());
  EXPECT_EQ(123u, value.getUnsigned());
  value.setString("uiae");
  EXPECT_FALSE(value.isUnsigned());
  EXPECT_TRUE(value.isString());
  EXPECT_EQ("uiae", value.getString());
  value.setNothing();
  EXPECT_FALSE(value.isString());
  EXPECT_TRUE(value.isNothing());
  value.setDefault();
  EXPECT_FALSE(value.isNothing());
  EXPECT_TRUE(value.isDefault());
}

TEST(LiteralValue, CanBePrinted) {
  std::string result;
  llvm::raw_string_ostream stream(result);

  LiteralValue().print(stream);
  stream << ' ';
  LiteralValue::createDefault().print(stream);
  stream << ' ';
  LiteralValue::createBoolean(true).print(stream);
  stream << ' ';
  LiteralValue::createBoolean(false).print(stream);
  stream << ' ';
  LiteralValue::createUnsigned(1234).print(stream);
  stream << ' ';
  LiteralValue::createString(R"( "uiae" nrtd)").print(stream);
  stream.flush();
  EXPECT_EQ(R"(nothing default true false 1234 " \"uiae\" nrtd")", result);
}

} // namespace
