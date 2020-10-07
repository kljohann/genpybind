#pragma once

#include "genpybind.h"

struct GENPYBIND(visible) Unrelated {};

struct GENPYBIND(visible) Base {
  bool from_base() const;
  bool hidden() const;
};

struct GENPYBIND(inline_base("::Base")) InlineBase : public Base {
  bool hidden() const;
};

struct GENPYBIND(visible) DerivedFromInlineBase : public InlineBase {};

struct Other {
  bool from_other() const;
};

struct GENPYBIND(inline_base("::Other")) InlineOther : public Base,
                                                       public Other {};

struct GENPYBIND(inline_base("Base", "Other")) InlineBoth : public Base,
                                                            public Other {};

struct GENPYBIND(visible) Derived : public Base {
  bool from_derived() const;
};

struct GENPYBIND(inline_base("Derived")) InlineDerived : public Derived {};

struct GENPYBIND(inline_base("Base")) InlineBaseKeepDerived : public Derived {};

struct GENPYBIND(inline_base("Base", "Derived")) InlineBaseAndDerived
    : public Derived {};

// `Base` isn't inlined, as the intermediate base class is hidden.
struct GENPYBIND(inline_base("Base"), hide_base("Derived")) InlineBaseHideDerived
    : public Derived {};

struct NumberOne {
  int number() const;
};

struct NumberTwo {
  int number() const;
};

struct GENPYBIND(inline_base("NumberOne", "NumberTwo")) InlineNumber
    : public NumberOne,
      public NumberTwo {};

namespace nested {

struct NestedBase {
  bool from_nested_base() const;
};

} // namespace nested

struct GENPYBIND(inline_base("::nested::NestedBase")) InlineNestedBase
    : public nested::NestedBase {};

struct GENPYBIND(inline_base("NestedBase")) InlineNestedBaseShort
    : public nested::NestedBase {};
