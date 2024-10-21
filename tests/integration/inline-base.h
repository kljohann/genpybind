// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <genpybind/genpybind.h>

#include <ostream>

struct GENPYBIND(visible) Base {
  bool from_base() const;
  bool hidden() const;
};

struct GENPYBIND(inline_base("::Base")) InlineBase : public Base {
  bool hidden() const;
};

struct GENPYBIND(visible) DerivedFromInlineBase : public InlineBase {};

struct GENPYBIND(inline_base(InlineBase)) InlineInlineBase : public InlineBase {
};

struct GENPYBIND(hide_base) HideBase : public Base {
  bool hidden() const;
};

struct GENPYBIND(inline_base(HideBase)) InlineHideBase : public HideBase {};

struct Other {
  bool from_other() const;
};

struct GENPYBIND(inline_base("::Other")) InlineOther : public Base,
                                                       public Other {};

struct GENPYBIND(inline_base("Base", "Other")) InlineBoth : public Base,
                                                            public Other {};

struct GENPYBIND(inline_base) InlineAll : public Base, public Other {};

struct GENPYBIND(visible) Derived : public Base {
  bool from_derived() const;
};

struct GENPYBIND(inline_base("Derived")) InlineDerived : public Derived {};

struct GENPYBIND(inline_base("Base")) InlineBaseKeepDerived : public Derived {};

struct GENPYBIND(inline_base("Base", "Derived")) InlineBaseAndDerived
    : public Derived {};

// `Base` isn't inlined, as the intermediate base class is hidden.
struct GENPYBIND(inline_base("Base"),
                 hide_base("Derived")) InlineBaseHideDerived : public Derived {
};

struct ConflictOne {
  int number() const;
};

struct ConflictTwo {
  int number() const;
};

struct GENPYBIND(inline_base("ConflictOne", "ConflictTwo")) InlineConflict
    : public ConflictOne,
      public ConflictTwo {};

namespace nested {

struct NestedBase {
  bool from_nested_base() const;
};

} // namespace nested

struct GENPYBIND(inline_base("::nested::NestedBase")) InlineNestedBase
    : public nested::NestedBase {};

struct GENPYBIND(inline_base("NestedBase")) InlineNestedBaseShort
    : public nested::NestedBase {};

template <typename Derived, typename T> struct CRTP : public T {
  bool from_crtp = true;
};

template <typename Derived, typename T>
struct GENPYBIND(inline_base(CRTP), hidden) SecondLevelOfCRTP
    : public CRTP<Derived, T> {
  bool from_second_level_crtp = true;
};

struct GENPYBIND(visible) TwoLevelCRTPNoInline
    : public SecondLevelOfCRTP<TwoLevelCRTPNoInline, Base> {};

struct GENPYBIND(inline_base(CRTP)) TwoLevelCRTPInlineFirst
    : public SecondLevelOfCRTP<TwoLevelCRTPInlineFirst, Base> {};

struct GENPYBIND(inline_base(SecondLevelOfCRTP)) TwoLevelCRTPInlineSecond
    : public SecondLevelOfCRTP<TwoLevelCRTPInlineSecond, Base> {};

struct GENPYBIND(inline_base(CRTP, SecondLevelOfCRTP)) TwoLevelCRTPInlineBoth
    : public SecondLevelOfCRTP<TwoLevelCRTPInlineBoth, Base> {};

template <typename T> struct NeqMixin {
  friend bool operator!=(T const &lhs, T const &rhs) { return !(lhs == rhs); }
};

template <typename T> struct NonsenseInvertMixin {
  T operator~() const { return T(-5); }
};

template <typename T> struct OstreamMixin {
  GENPYBIND(expose_as(__str__))
  friend std::ostream &operator<<(std::ostream &os, const T &instance) {
    return os << "The value is " << instance.value;
  }
};

struct GENPYBIND(inline_base) WithOperatorMixins
    : NeqMixin<WithOperatorMixins>,
      NonsenseInvertMixin<WithOperatorMixins>,
      OstreamMixin<WithOperatorMixins> {
  int value;
  WithOperatorMixins(int value) : value(value) {}
  friend bool operator==(WithOperatorMixins const &lhs,
                         WithOperatorMixins const &rhs) {
    return lhs.value == rhs.value;
  }
};
