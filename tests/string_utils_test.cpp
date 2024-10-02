// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/string_utils.h"

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringRef.h>

#include <gtest/gtest.h>

#include <initializer_list>

namespace {

using namespace ::genpybind;

TEST(MakeValidIdentifier, ReplacesInvalidCharactersByUnderscores) {
  struct Example {
    llvm::StringRef input;
    llvm::StringRef expected;
  };
  for (auto example : std::initializer_list<Example>{
           {"", ""},
           {"_", "_"},
           {"<,&.", "_"},
           {"<,&.ui-ae.&,>", "_ui_ae_"},
           {"Something<int>", "Something_int_"},
           {"Some::Thing", "Some_Thing"},
           {"::Some::Thing", "_Some_Thing"},
           {"0123uiae", "_uiae"},
           {"x0123", "x0123"},
       }) {
    llvm::SmallString<64> name{example.input};
    makeValidIdentifier(name);
    EXPECT_LE(name.size(), example.input.size());
    EXPECT_EQ(example.expected.str(), name.str().str()) << example.input.str();
  }
}

} // namespace
