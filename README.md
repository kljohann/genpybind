<!--
SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>

SPDX-License-Identifier: MIT
-->

# genpybind

*Autogeneration of Python bindings from manually annotated C++ headers*

Genpybind is a tool based on [clang][] that automatically generates code to
expose a C++ API as a Python extension module via [pybind11][].  Say goodbye to the
tedious task of writing and updating binding code by hand!  Genpybind ensures
that your Python bindings always stay in sync with your C++ API, complete with
docstrings, parameter names, and default arguments.  This is especially valuable
for still-evolving APIs where manual bindings can quickly become outdated.

While genpybind does require some manual hints in the form of unobtrusive
annotation macros[^1], it results in a self-contained header file that concisely
describes both the C++ and Python interfaces of your library.  This approach
keeps you in control and requires less heuristics in genpybind's implementation,
thereby reducing complexity.  Though it does require the ability to modify the
original interface declarations, so code which is not under your control needs
to fall back on manually written bindings.

Besides the main use case of exposing a C++ API to Python, genpybind has proven useful
*during* C++ library development:
- It enables interactive exploration of a library's API via the Python REPL.
- This exploration can form the basis for unit tests using Python's
  low-boilerplate testing frameworks like [pytest][].
- And maybe most importantly, it enables hassle-free *property-based testing*
  via [hypothesis][], which still has no C++-native equivalent.

[clang]: https://clang.llvm.org/
[hypothesis]: https://hypothesis.readthedocs.io
[pybind11]: https://github.com/pybind/pybind11
[pytest]: https://doc.pytest.org/

## Example

To expose a C++ interface via a Python module, `GENPYBIND` annotations are added
to the C++ declarations:

```cpp
#pragma once

#include <genpybind/genpybind.h>

namespace readme GENPYBIND(visible) {

/// Describes how the output will taste.
enum class Flavor {
  /// Like you would expect.
  bland,
  /// It tastes different.
  fruity,
};

/// A contrived example.
class Example {
public:
  static constexpr int GENPYBIND(hidden) not_exposed = 10;

  /// Do a complicated calculation.
  int calculate(Flavor flavor = Flavor::fruity) const;

  GENPYBIND(getter_for(something))
  int getSomething() const;

  GENPYBIND(setter_for(something))
  void setSomething(int value);

private:
  int m_value = 0;
};

} // namespace readme
```

The resulting module can then be used like this:

```python
>>> import readme as m
>>> obj = m.Example()
>>> obj.something
0
>>> obj.something = 42
>>> obj.something
42
>>> obj.calculate()  # default argument
-42
>>> obj.calculate(m.Flavor.bland)
42
>>> print(m.Example.__doc__)
A contrived example.
>>> print(m.Flavor.__doc__)
Describes how the output will taste.

Members:

  bland : Like you would expect.

  fruity : It tastes different.
>>> help(obj.calculate)
Help on method calculate in module readme:

calculate(...) method of readme.Example instance
    calculate(self: readme.Example, flavor: readme.Flavor = <Flavor.fruity: 1>) -> int

    Do a complicated calculation.
```

For the example presented above, `genpybind` will generate code equivalent to
the following: (Note that docstrings, argument names and default arguments work
out of the box, without extra annotations.)

```cpp
void expose_context_readme_Flavor(py::enum_<readme::Flavor>& context);
void expose_context_readme_Example(py::class_<readme::Example>& context);

PYBIND11_MODULE(readme, root) {
  auto context_readme_Flavor = py::enum_<readme::Flavor>(
      root, "Flavor", "Describes how the output will taste.");
  auto context_readme_Example = py::class_<readme::Example>(
      root, "Example", "A contrived example.");

  expose_context_readme_Flavor(context_readme_Flavor);
  expose_context_readme_Example(context_readme_Example);
}

void expose_context_readme_Flavor(py::enum_<readme::Flavor>& context) {
  context.value("bland", readme::Flavor::bland, "Like you would expect.");
  context.value("fruity", readme::Flavor::fruity, "It tastes different.");
}

void expose_context_readme_Example(py::class_<readme::Example>& context) {
  context.def(py::init<>(), "");
  context.def(py::init<const readme::Example&>(), "", py::arg(""));
  context.def("calculate",
              py::overload_cast<readme::Flavor>(&readme::Example::calculate, py::const_),
              "Do a complicated calculation.",
              py::arg("flavor") = readme::Flavor::fruity);
  context.def_property(
      "something",
      py::overload_cast<>(&readme::Example::getSomething, py::const_),
      py::overload_cast<int>(&readme::Example::setSomething));
}
```

# Getting started

To use genpybind in your project, the simplest approach is through
[scikit-build-core][].  Since genpybind is [available on PyPI][pypi], setup is
similar to a standard pybind11 extension module, except that you need to:

1. add genpybind as an additional build dependency, and
2. use `genpybind_add_module` instead of `pybind11_add_module`
   (see [`tools/genpybind.cmake`](./tools/genpybind.cmake)).

```toml
# In pyproject.toml:

[build-system]
requires = ["scikit-build-core", "pybind11", "genpybind"]
```

```cmake
# In CMakeLists.txt:

set(PYBIND11_NEWPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)
find_package(genpybind CONFIG REQUIRED)

genpybind_add_module(
  your_module MODULE
  HEADER include/your_module.h
  src/a.cpp src/b.cpp src/c.cpp
)
```

See the [example project](./example-project) for a complete implementation.

<details>
<summary>Using genpybind without a separate header file</summary>

```cpp
// In your_module.cpp:

#include <genpybind/genpybind.h>

double square(double x) GENPYBIND(visible) { return x * x; }
```

```cmake
# In CMakeLists.txt:

genpybind_add_module(
  your_module MODULE
  HEADER your_module.cpp
  your_module.cpp
)
```

</details>

<details>
<summary>Link against existing library (which might also be consumed by other C++ targets)</summary>

```cmake
# In CMakeLists.txt:

add_library(some_library SHARED src/a.cpp src/b.cpp src/c.cpp)
# Add dependency to allow `#include <genpybind/genpybind.h>`
target_link_libraries(some_library PUBLIC genpybind::genpybind)
genpybind_add_module(
  py_some_library MODULE
  LINK_LIBRARIES some_library
  NUM_BINDING_FILES 1
  HEADER include/some_library.h
)
```

</details>

[scikit-build-core]: https://scikit-build-core.readthedocs.io/
[pypi]: https://pypi.org/project/genpybind/

# Implementation

The current implementation is a prototype based on clang's `libtooling` API.
A [previous proof-of-concept Python implementation][legacy] I developed at the
[Electronic Vision(s) Group][visions] ran into limits of the `libclang` bindings
and required a patched LLVM/clang build.  Still, it's used successfully in the
experiment software stack of their neuromorphic computing platform, i.e., the
described approach is viable for an existing code base.

[legacy]: https://github.com/kljohann/genpybind-legacy
[visions]: http://www.kip.uni-heidelberg.de/vision/

The current iteration still lacks some polishing.  Some known shortcomings
remain, the documentation is still lacking, and build support (and automated
testing) on different platforms is pending.

## Known defects and shortcomings

- Documentation is minimal at the moment.  If you want to look at example use-cases the
  [integration tests](./tests/integration) might provide a starting point.
- Expressions and types in default arguments, return values, or
  `GENPYBIND_MANUAL` instructions are not consistently expanded to their fully
  qualified form.  As a workaround it is suggested to use the fully-qualified
  name where necessary.

## Changes compared to the Python prototype

Apart from the less involved build process, the current implementation comes
with many new features and improvements.  For example, considerably better
error messages.  As a small price to pay there are several breaking changes:

- `opaque` is now known as `expose_here`.
- `expose_as(__repr__)` (or `__str__`) should be used in place of `stringstream`.
- `tag` should be replaced by `only_expose_in`.
- `inline_base` no longer supports globs/wildcards.
- `accessor_for` is no longer supported, use `getter_for`/`setter_for` instead.
- `writeable` is no longer supported, use `readonly` instead.

# Building from source

So far, the only tested platform is [Fedora Workstation][fedora] 40, though
at least Debian has been tested in the past.  You should be able to adapt the
instructions to other distributions.

[fedora]: https://fedoraproject.org/workstation/
[direnv]: https://github.com/direnv/direnv/

1. Check out the repo, the following commands should be run from the repo root.
2. Install dependencies:
   ```
   dnf install \
     llvm-devel clang-devel gtest-devel gmock-devel cmake ninja-build \
     python3-devel python3-pip pybind11-devel
   ```
3. Set up the build:
   ```
   cmake -B ./build -G Ninja .
   ```
4. Build and install (adapt the prefix accordingly):
   ```
   cmake --build ./build
   cmake --install ./build --prefix ~/.local
   ```

See `genpybind_add_module` in [`tools/genpybind.cmake`](./tools/genpybind.cmake)
and how it's [used in an example project](./example-project/CMakeLists.txt) for
how to integrate genpybind into your build.  Depending on the prefix you used
during installation, you might need to specify the location of
`genpybindConfig.cmake` explicitly in downstream builds, e.g.:
`cmake … -Dgenpybind_DIR="$HOME/.local/share/cmake/genpybind" …`.

## Extra steps (for development)
1. Inside a virtual environment (e.g., via [direnv][] with `layout python`),
   install the Python dependencies (used in tests):
   ```
   pip install -r requirements.txt
   ```
2. Set up [pre-commit][]:
   ```
   pip install pre-commit
   pre-commit install
   ```
3. Build and run the tests:
   ```
   PYTHONPATH=$PWD/build/tests ninja -C build test
   ```
   If you use direnv, it's convenient to add `path_add PYTHONPATH build/tests`
   to your `.envrc`.

[pre-commit]: https://pre-commit.com/

# Annotations

Top-level declarations are only exposed via the Python bindings (“visible”) if
they have a `GENPYBIND(…)` annotation.  Nested declarations, such as member
variables and member functions inherit the visibility of their parent
by default.

There are several possible modifiers that can be passed as arguments to
`GENPYBIND(…)` to affect how and where a declaration is exposed or to make use
of advanced `pybind11` features.

## Where to place the `GENPYBIND(…)` annotation

Behind the scenes, the `GENPYBIND` macro expands to an [attribute][attributes],
in particular the older GNU extension syntax `__attribute__` at this time.
Consequently, you can consult the [GCC documentation][gnu-attributes] on details
w.r.t. attribute placement.  Here are some common examples for your convenience:

```cpp
struct GENPYBIND(visible) Example {
  void hidden_method() GENPYBIND(hidden);

  GENPYBIND(readonly)
  int readonly_field = 3;
};
enum class GENPYBIND(visible) Enum {};
void example() GENPYBIND(visible);

namespace readme GENPYBIND(visible) {}
```

[attributes]: https://en.cppreference.com/w/cpp/language/attributes
[gnu-attributes]: https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html

TODO: Describe annotation argument types and when quotes can be omitted for
string arguments.

## General modifiers

### `visible` and `hidden`

`visible` and `hidden` can be used to override the default visibility of
a declaration.  By default, top-level declarations are hidden, and nested
declarations inherit the visibility of their parent.  So one has to explicitly
“opt-in” to exposing a declaration.  Any use of `GENPYBIND(…)` annotations (even
without arguments) implies `visible`, unless `hidden` is used explicitly.

Namespaces are a special case: By default, they have no effect on the visibility
of contained declarations and other attributes on namespaces do not imply `visible`.
However, an explicit `visible` annotation on a namespace can be used to make all
nested declarations visible by default.  The `hidden` keyword can then be used to
exclude individual declarations again.

```cpp
struct GENPYBIND() A {
  GENPYBIND(hidden)
  int some_field;
};

struct GENPYBIND(visible) B {};

// This would not have been exposed anyways, but we can
// include `hidden` to document our intent explicitly.
struct GENPYBIND(hidden) C {};

namespace example GENPYBIND(visible) {
struct Example {}; // Visible, even though there is no annotation.
}
```

### `expose_as`

By default a declaration will be exposed using the name of its C++ identifier.
`expose_as` can be used to choose a different name in the Python bindings:

```cpp
struct GENPYBIND(expose_as(Example)) example {};
```

This can also be used to define [special methods][] like `__repr__` or `__hash__`:

```cpp
GENPYBIND(expose_as(__hash__))
int hash() const;
```

[special methods]: https://docs.python.org/3.13/reference/datamodel.html#special-method-names

## `GENPYBIND_MANUAL` (manual bindings)

You can always fall back on hand-written bindings that is embedded in the
generated binding code.  This can be a convenient escape hatch for pybind11
features that are not (yet) supported by genpybind.

### Inside structs and classes

Inside structs and classes, `parent` can be used to refer to the corresponding
`pybind11::class_` instance.  If you need to access members of the parent class,
you can use `GENPYBIND_PARENT_TYPE` instead of directly referring to its name.
This is necessary, as the definitions is not yet complete at the point of
the macro.

```cpp
struct GENPYBIND(visible) Example {
  bool values[2] GENPYBIND(hidden) = {false, false};

  GENPYBIND_MANUAL({
    using Example = GENPYBIND_PARENT_TYPE;
    parent.def("__getitem__",
               [](Example& self, bool key) { return self.values[key]; });
    parent.def("__setitem__", [](Example& self, bool key, bool value) {
      self.values[key] = value;
    });
  })
```

### At the top level, as a preamble or `postamble` to the binding code

If `GENPYBIND_MANUAL` is usde at the top-level, the contained code is emitted
before all auto-generated binding code.  This can be useful to, e.g., import
another module (see the `only_expose_in` annotation on namespaces) that is used
in function signatures:

```cpp
GENPYBIND_MANUAL({
  ::pybind11::module::import("common");
})
```

The `postamble` modifier can be used to embed code *after* all auto-generated
binding code, e.g., to dynamically patch the generated bindings:

```cpp
GENPYBIND(postamble)
GENPYBIND_MANUAL({
  auto example = parent.attr("Example");
  // …patch example…
})
```

Note that `parent` can be used to refer to the corresponding `pybind11::module`.

In general, different `GENPYBIND_MANUAL` blocks are emitted in the order in
which they were defined.

## Namespaces

For all accessible headers, the annotations of a particular header have to
match, as long as the namespace contains at least one annotated declaration
exposed via the bindings:

```cpp
namespace example GENPYBIND(module) {
struct GENPYBIND(visible) Example {};
}

// OK: No annotated declarations
namespace example {
struct Hidden {};
}

// OK: Same annotations
namespace example GENPYBIND(module) {
struct GENPYBIND(visible) Other {};
}
```

### `module`

Namespaces can be annotated using `module` to turn them into sub-modules of the
generated Python module.  Namespaces that do not have this annotation have no
effect on the module hierarchy of the generated Python bindings.

E.g., if `readme` is the name of the top-level module, `X` in the following
example would be exposed as `readme.nested.X`:
```cpp
namespace nested GENPYBIND(module) {
class GENPYBIND(visible) X {};
} // namespace nested
```

### `only_expose_in`

When generating multiple Python libraries, `only_expose_in` should be used to
only expose declarations in the corresponding module.  When used on a namespace,
all nested declarations are only exposed if one of the arguments to
`only_expose_in` matches the name of the top-level module, which is derived from
the basename of the header file passed to genpybind.  For example:

```cpp
// In common.h:
namespace common GENPYBIND(only_expose_in(common)) {
struct GENPYBIND(visible) Example {};
}

// In downstream.h:
# include <…/common.h>
namespace downstream GENPYBIND(only_expose_in(downstream)) {
void sink(common::Example input) GENPYBIND(visible);
}
```

`Example` is only available via the `common` module, instead of being duplicated
/ exposed twice:
```python
from common import Example
from downstream import sink
sink(Example())
```

## Enums

[pybind11-classes]: https://pybind11.readthedocs.io/en/stable/classes.html

### `arithmetic`

The `arithmetic` modifier can be used to expose arithmetic operations on the
generated enum by passing the [`pybind11::arithmetic()`][pybind11-classes]
tag to the `pybind11::enum_` constructor:

```cpp
enum GENPYBIND(arithmetic) Access { READ = 4, WRITE = 2, EXECUTE = 1 };
```

### `export_values`

The `export_values` modifier controls whether enumerators are available in the
parent scope.  By default, this is only the case for unscoped enums.

In the following example defaults are overridden s.t. `RED` is only available as
`example.Color.RED` and `HIGH` is available as `example.HIGH`:

```cpp
enum GENPYBIND(export_values(false)) Color { RED, GREEN, BLUE };

enum class GENPYBIND(export_values) Level { HIGH, MEDIUM, LOW };
```

## Structs and classes

### `dynamic_attr` (dynamic attributes)

The `dynamic_attr` modifier can be used to allow additional attributes to be set
at runtime, by passing the [`pybind11::dynamic_attr()`][pybind11-classes] tag to
the `pybind11::class_` constructor.  I.e., in the following example,
`thing.unknown_attribute = 5` would work on an instance `thing = Thing()`.

```cpp
struct GENPYBIND(dynamic_attr) Thing {};
```

### `hide_base`

By default, base classes included as template parameters of `pybind11::class_`,
which has the effect that the inheritance relationship is represented on the
Python side.  If that's not what you want, you can opt out using `hide_base`:

```cpp
struct GENPYBIND(hide_base) HideAll : common::Base, Base2, Base3 {};
struct GENPYBIND(hide_base("common::Base")) HideOne : common::Base, Base2, Base3 {};
struct GENPYBIND(hide_base("Base2", "Base3")) HideTwo : common::Base, Base2, Base3 {};
```

### `holder_type`

The `holder_type` modifier can be used to set the [holder type][pybind11-smart]
used to manage references to objects (defaults to `std::unique_ptr<…>`).

```cpp
struct GENPYBIND(holder_type("std::shared_ptr<Example>")) Example
    : public std::enable_shared_from_this<Example> {
  std::shared_ptr<Example> clone();
};
```

### `implicit_conversion` (on constructor)

The `implicit_conversion` modifier can be added to converting constructors to
denote that the corresponding conversion should be registered as an [implicit
conversion][pybind11-impl-conv] via `pybind11::implicitly_convertible<…>`:

```cpp
struct GENPYBIND(visible) Implicit {
  explicit Implicit(int value) GENPYBIND(implicit_conversion);
  Implicit(Example example) GENPYBIND(implicit_conversion);
};
```

[pybind11-impl-conv]: https://pybind11.readthedocs.io/en/stable/advanced/classes.html#implicit-conversions

### `inline_base`

Similar to `hide_base` described above, `inline_base` has the effect that the
inheritance relationship is not represented on the Python side.  In addition,
declarations nested in the base class are pulled in, as if they were defined in
the current class.  This is useful for mixins / [CRTP][] code.

```cpp
struct GENPYBIND(inline_base) InlineAll : common::Base, Base2, Base3 {};
struct GENPYBIND(inline_base("common::Base")) InlineOne : common::Base, Base2, Base3 {};
struct GENPYBIND(inline_base("Base2", "Base3")) InlineTwo : common::Base, Base2, Base3 {};
```

[CRTP]: https://en.cppreference.com/w/cpp/language/crtp

## Templates

Explicit template instantiations have the same visibility as the corresponding
template by default.  They can be selectively exposed by adding any `GENPYBIND`
annotation.  `expose_as` can be used to rename individual instantiations.  Else,
a fallback name is generated by replacing special characters with underscores.
E.g., `Some<int>` is exposed as `Some_int_`.

```cpp
template <typename T> struct ExposeSome {};
extern template struct GENPYBIND(expose_as(IntSomething))
    ExposeSome<int>; // selectively exposed
extern template struct ExposeSome<double>; // not exposed

template <typename T> struct GENPYBIND(visible) ExposeAll {};
extern template struct ExposeAll<int>;
extern template struct GENPYBIND(expose_as(BoolEx)) ExposeAll<bool>;
```

## Type aliases (`using` and `typedef`)

Type aliases are hidden (i.e., not exposed) by default and they do not inherit
the default visibility.  If they are marked as `visible`, a simple alias is
created in the Python bindings by assigning a reference to the alias target to
an attribute.  I.e., `using` in the following example is equivalent to the
assignment `X.Alias = Y` in Python.

```cpp
struct GENPYBIND(visible) X {
  using Alias GENPYBIND(visible) = Y;
};
```

Note: [Using declarations][using-decl] _are not type aliases_.
[using-decl]: https://en.cppreference.com/w/cpp/language/using_declaration

### `expose_here`

The `expose_here` modifier can be used to influence where the alias _target_ is
exposed.  This can be useful to, e.g., pull in / “transplant” declarations from
another module or a nested scope.  Or to selectively expose single-purpose
template instances in a particular scope.  The corresponding declarations are
then no longer exposed in their original declaration context.

```cpp
struct GENPYBIND(visible) Example {
  using tag_type GENPYBIND(expose_here) = common::Tag<Example>;
};
```

### `encourage`

The `encourage` modifier can be used to make the _target_ of a type alias
visible in its original scope.  This can be useful to selectively instantiate
templates.  (This implies an “assignment”-style alias on the Python side, as
described above.)

```cpp
struct GENPYBIND(visible) Example {
  using value_type GENPYBIND(encourage) =
      common::Ranged<int, common::Gt<0>, common::Lt<5>>;
};
```

## Functions and member functions / methods

### `keep_alive`

The `keep_alive` modifier corresponds to pybind11's [call
policy][pybind11-keep-alive] of the same name.  It can be used to indicate the
intended lifetime of objects passed to or returned from (member) functions:
`keep_alive(<bound>, <who>)` means that `<who>` should be kept alive at least as
long as `<bound>`.  `<who>` and `<bound>` can either be the name of a function
parameter, `return` (the function's return value), or `this` (the instance
a member function is called on).  Behind the scenes this is translated into the
index-based notation used by pybind11.


[pybind11-keep-alive]: https://pybind11.readthedocs.io/en/stable/advanced/functions.html#keep-alive

```cpp
struct GENPYBIND(visible) Container {
  GENPYBIND(keep_alive(this, resource))
  Container(Resource *resource);
};
```

### `noconvert`

`noconvert` can be used to [disable implicit conversion][pybind11-noconvert] for
arguments passed via certain function parameters (multiple parameter names can
be specified):

[pybind11-noconvert]: https://pybind11.readthedocs.io/en/stable/advanced/functions.html#non-converting-arguments

```cpp
GENPYBIND(noconvert(value))
double no_ints_please(double value);
```

### `required`

The `required` modifier can be used to [prohibit `None` arguments][pybind11-none]
for certain function parameters (multiple parameter names can be specified).
It is equivalent to calling `.none(false)` on the corresponding `pybind11::arg` object.

[pybind11-none]: https://pybind11.readthedocs.io/en/stable/advanced/functions.html#allow-prohibiting-none-arguments

```cpp
GENPYBIND(required(Example))
void required(Example *example)
```

## `return_value_policy`

The `return_value_policy` modifier can be used to set any [return value
policy][pybind11-rvp] supported by pybind11:

[pybind11-rvp]: https://pybind11.readthedocs.io/en/stable/advanced/functions.html#return-value-policies

```cpp
struct GENPYBIND(visible) Example {
  GENPYBIND(return_value_policy(reference_internal))
  Thing& thing();
};
```

### `getter_for` / `setter_for` (member functions only)

`getter_for` and `setter_for` can be used to expose member function as Python properties:

```cpp
struct GENPYBIND(visible) Example {
  GENPYBIND(getter_for(value))
  int getValue() const;

  GENPYBIND(setter_for(value))
  void setValue(int value);

  GENPYBIND(getter_for(readonly))
  bool getReadonly() const;
};
```

## Operators

[Special methods][special methods] like `__eq__` are emitted for unary (`+`,
`-`, `!`) and binary (`+`, `-`, `*`, `/`, `%`, `^`, `&`, `|`, `<`, `>`, `<<`,
`>>`, `==`, `!=`, `<=`, `>=`) operators defined on classes.  Operators can be
either member functions or free functions in a the associated namespace of the
class (found via ADL).  Where necessary, operators and parameters are switched:
E.g., `operator<(int, T)` cannot be exposed as `int.__lt__` so it is exposed as
`T.__gt__` instead.

```cpp
struct GENPYBIND(visible) Number {
  bool operator==(Number other) const { return value == other.value; }
  friend bool operator<(const Number &lhs, const Number &rhs) {
    return lhs.value < rhs.value;
  }
  friend bool operator>(int lhs, Number rhs) { return lhs > rhs.value; }
};
```

TODO: Support for the spaceship operator is pending.

### `std::ostream` operators

`std::ostream` operators are only exposed when opted in via, e.g.,
`expose_as(__repr__)`:

```cpp
struct GENPYBIND(visible) Example {
  GENPYBIND(expose_as(__str__))
  friend std::ostream& operator<<(std::ostream& os, const Example& value);
};
```

## Variables and member variables / fields

Variables are exposed using `def_readonly` and `def_readwrite` (and their
`_static` variants) according to their constness.

### `readonly`

The `readonly` modifier can be used if a non-const variable should be exposed as
read-only:

```cpp
struct GENPYBIND(visible) Example {
  GENPYBIND(readonly)
  int readonly_field = 0;
};
```

# License

genpybind is provided under the MIT license.  By using, distributing, or
contributing to this project, you agree to the terms and conditions of this
license.  See the [license file](LICENSES/MIT.txt) for details.

genpybind links against the LLVM and clang projects, which are licensed under
the Apache License v2.0 with LLVM Exceptions.  For details, see the included
[license file](LICENSES/LicenseRef-LLVM.txt).  Binary distributions of genpybind
may incorporate unmodified parts of LLVM and/or clang through static linking.

---

[^1]: During normal compilation these macros have no effect on the generated code, as they are defined
  to be empty.  The annotation system is implemented using the `annotate` attribute specifier, which
  is available as a GNU language extension via `__attribute__((...))`.  As the annotation macros
  only have to be parsed by clang and are empty during normal compilation the annotated code can
  still be compiled by any C++ compiler.  See [genpybind.h][genpybind.h] for the definition of
  the macros.

[genpybind.h]: ./public/genpybind/genpybind.h
