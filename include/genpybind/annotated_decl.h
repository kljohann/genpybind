// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "genpybind/annotations/annotation.h"

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>

#include <cassert>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace clang {
class Decl;
class NamedDecl;
class TypedefNameDecl;
class TagDecl;
class LambdaExpr;
} // namespace clang

namespace genpybind {

class AnnotationStorage;

bool hasAnnotations(const clang::Decl *decl, bool allow_empty = true);

/// Return spelling derived from the name of the identifier that names
/// the declaration.
std::string getSpelling(const clang::NamedDecl *decl);

const clang::TagDecl *aliasTarget(const clang::TypedefNameDecl *decl);

/// Applies some annotations of this declaration to the alias target.
/// This is used to propagate spelling and visibility in the case of
/// `expose_here` type aliases.
void propagateAnnotations(AnnotationStorage &annotations,
                          const clang::TypedefNameDecl *decl);

struct NamedDeclAttrs {
  /// Alternative identifier to use when exposing the declaration.
  std::string spelling;
  /// Explicitly specified visibility of the declaration.
  /// If this is `std::nullopt` the effective visibility will be determined
  /// based on the visibility of the parent declaration.
  std::optional<bool> visible;

  static bool supports(const clang::NamedDecl *decl);
  friend bool operator==(NamedDeclAttrs const &,
                         NamedDeclAttrs const &) = default;
};

bool processAnnotation(const clang::NamedDecl *decl,
                       const annotations::Annotation &annotation,
                       NamedDeclAttrs &attrs);

struct NamespaceDeclAttrs {
  bool module = false;
  std::vector<std::string> only_expose_in;

  static bool supports(const clang::NamedDecl *decl);
  friend bool operator==(NamespaceDeclAttrs const &,
                         NamespaceDeclAttrs const &) = default;
};

bool processAnnotation(const clang::NamedDecl *decl,
                       const annotations::Annotation &annotation,
                       NamespaceDeclAttrs &attrs);

struct EnumDeclAttrs {
  bool arithmetic = false;
  std::optional<bool> export_values;

  static bool supports(const clang::NamedDecl *decl);
  friend bool operator==(EnumDeclAttrs const &,
                         EnumDeclAttrs const &) = default;
};

bool processAnnotation(const clang::NamedDecl *decl,
                       const annotations::Annotation &annotation,
                       EnumDeclAttrs &attrs);

struct RecordDeclAttrs {
  bool dynamic_attr = false;
  llvm::SmallPtrSet<const clang::TagDecl *, 1> hide_base;
  llvm::SmallPtrSet<const clang::TagDecl *, 1> inline_base;
  std::string holder_type;

  static bool supports(const clang::NamedDecl *decl);
  friend bool operator==(RecordDeclAttrs const &,
                         RecordDeclAttrs const &) = default;
};

bool processAnnotation(const clang::NamedDecl *decl,
                       const annotations::Annotation &annotation,
                       RecordDeclAttrs &attrs);

struct TypedefNameDeclAttrs {
  /// If the underlying type has default visibility, make it visible explicitly.
  bool encourage = false;
  /// Expose the underlying type at the location of the type alias instead.
  bool expose_here = false;

  static bool supports(const clang::NamedDecl *decl);
  friend bool operator==(TypedefNameDeclAttrs const &,
                         TypedefNameDeclAttrs const &) = default;
};

bool processAnnotation(const clang::NamedDecl *decl,
                       const annotations::Annotation &annotation,
                       TypedefNameDeclAttrs &attrs);

struct FieldOrVarDeclAttrs {
  bool readonly = false;
  const clang::LambdaExpr *manual_bindings = nullptr;
  bool postamble = false;

  static bool supports(const clang::NamedDecl *decl);
  friend bool operator==(FieldOrVarDeclAttrs const &,
                         FieldOrVarDeclAttrs const &) = default;
};

bool processAnnotation(const clang::NamedDecl *decl,
                       const annotations::Annotation &annotation,
                       FieldOrVarDeclAttrs &attrs);

/// Overloaded operators do not support all function annotations and are thus
/// handled separately.  One exception is call operators, which are represented
/// using `MethodDeclAttrs`s.
struct OperatorDeclAttrs {
  static bool supports(const clang::NamedDecl *decl);
  friend bool operator==(OperatorDeclAttrs const &,
                         OperatorDeclAttrs const &) = default;
};

bool processAnnotation(const clang::NamedDecl *decl,
                       const annotations::Annotation &annotation,
                       OperatorDeclAttrs &attrs);

struct ConversionFunctionDeclAttrs {
  static bool supports(const clang::NamedDecl *decl);
  friend bool operator==(ConversionFunctionDeclAttrs const &,
                         ConversionFunctionDeclAttrs const &) = default;
};

bool processAnnotation(const clang::NamedDecl *decl,
                       const annotations::Annotation &annotation,
                       ConversionFunctionDeclAttrs &attrs);

struct MethodDeclAttrs {
  llvm::SmallSet<std::string, 1> getter_for;
  llvm::SmallSet<std::string, 1> setter_for;

  static bool supports(const clang::NamedDecl *decl);
  friend bool operator==(MethodDeclAttrs const &,
                         MethodDeclAttrs const &) = default;
};

bool processAnnotation(const clang::NamedDecl *decl,
                       const annotations::Annotation &annotation,
                       MethodDeclAttrs &attrs);

struct ConstructorDeclAttrs {
  bool implicit_conversion = false;

  static bool supports(const clang::NamedDecl *decl);
  friend bool operator==(ConstructorDeclAttrs const &,
                         ConstructorDeclAttrs const &) = default;
};

bool processAnnotation(const clang::NamedDecl *decl,
                       const annotations::Annotation &annotation,
                       ConstructorDeclAttrs &attrs);

struct FunctionDeclAttrs {
  llvm::SmallVector<std::pair<unsigned, unsigned>, 1> keep_alive;
  llvm::SmallSet<unsigned, 1> noconvert;
  llvm::SmallSet<unsigned, 1> required;
  std::string return_value_policy;

  static bool supports(const clang::NamedDecl *decl);
  friend bool operator==(FunctionDeclAttrs const &,
                         FunctionDeclAttrs const &) = default;
};

bool processAnnotation(const clang::NamedDecl *decl,
                       const annotations::Annotation &annotation,
                       FunctionDeclAttrs &attrs);

/// Holds and owns annotations s.t. they do not have to be processed multiple
/// times.  Currently only named declarations are checked for annotations.
class AnnotationStorage {
  template <typename T>
  using Map = std::unordered_map<const clang::NamedDecl *, T>;
  using AttrTypes =
      std::tuple<NamedDeclAttrs, NamespaceDeclAttrs, EnumDeclAttrs,
                 RecordDeclAttrs, TypedefNameDeclAttrs, FieldOrVarDeclAttrs,
                 OperatorDeclAttrs, ConversionFunctionDeclAttrs,
                 MethodDeclAttrs, ConstructorDeclAttrs, FunctionDeclAttrs>;

  template <typename T> struct MapsOf;
  template <typename... Ts> struct MapsOf<std::tuple<Ts...>> {
    using type = std::tuple<Map<Ts>...>;
  };

  // clang-format off
  MapsOf<AttrTypes>::type attrs_by_decl;
  // clang-format on

public:
  void insert(const clang::NamedDecl *decl);

  template <typename T>
  std::optional<T> get(const clang::NamedDecl *decl) const {
    const auto &map = std::get<Map<T>>(attrs_by_decl);
    if (auto it = map.find(decl); it != map.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  template <typename T, typename UpdateOp>
  void update(const clang::NamedDecl *decl, UpdateOp op) {
    assert(decl != nullptr && T::supports(decl));
    op(std::get<Map<T>>(attrs_by_decl)[decl]);
  }

  template <typename T> T lookup(const clang::NamedDecl *decl) const {
    assert(decl != nullptr && T::supports(decl));
    if (auto attrs = get<T>(decl)) {
      return *attrs;
    }
    return T();
  }

  bool equal(const clang::NamedDecl *left, const clang::NamedDecl *right) const;

  auto has(const clang::NamedDecl *decl) const {
    return std::get<Map<NamedDeclAttrs>>(attrs_by_decl).contains(decl);
  }

  auto size() const {
    return std::get<Map<NamedDeclAttrs>>(attrs_by_decl).size();
  }
};

} // namespace genpybind
