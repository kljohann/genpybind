// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/annotated_decl.h"

#include "genpybind/annotations/literal_value.h"
#include "genpybind/annotations/parser.h"
#include "genpybind/diagnostics.h"
#include "genpybind/lookup_context_collector.h"
#include "genpybind/string_utils.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Attr.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/DeclarationName.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/STLForwardCompat.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>
#include <functional>
#include <iterator>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace clang {
struct PrintingPolicy;
} // namespace clang

using namespace genpybind;

using annotations::Annotation;
using annotations::AnnotationKind;
using annotations::LiteralValue;
using annotations::Parser;

static const llvm::StringRef k_lozenge = "◊";

bool genpybind::hasAnnotations(const clang::Decl *decl, bool allow_empty) {
  return llvm::any_of(
      decl->specific_attrs<clang::AnnotateAttr>(), [&](const auto *attr) {
        llvm::StringRef annotation_text = attr->getAnnotation();
        if (!annotation_text.starts_with(k_lozenge))
          return false;
        if (allow_empty)
          return true;
        return !annotation_text.drop_front(k_lozenge.size()).ltrim().empty();
      });
}

static Parser::Annotations parseAnnotations(const clang::Decl *decl) {
  Parser::Annotations annotations;

  for (const auto *attr : decl->specific_attrs<clang::AnnotateAttr>()) {
    llvm::StringRef annotation_text = attr->getAnnotation();
    if (!annotation_text.consume_front(k_lozenge))
      continue;
    handleAllErrors(Parser::parseAnnotations(annotation_text, annotations),
                    [attr, decl](const Parser::Error &error) {
                      clang::ASTContext &context = decl->getASTContext();
                      const clang::SourceLocation loc =
                          context.getSourceManager().getExpansionLoc(
                              attr->getLocation());
                      error.report(loc, context.getDiagnostics());
                    });
  }

  return annotations;
}

static llvm::StringRef friendlyName(const clang::NamedDecl *decl) {
  if (NamespaceDeclAttrs::supports(decl))
    return "namespace";
  if (EnumDeclAttrs::supports(decl))
    return "enum";
  if (RecordDeclAttrs::supports(decl))
    return "class";
  if (TypedefNameDeclAttrs::supports(decl))
    return "type alias";
  if (FieldOrVarDeclAttrs::supports(decl))
    return "variable";
  if (OperatorDeclAttrs::supports(decl))
    return "operator";
  if (ConversionFunctionDeclAttrs::supports(decl))
    return "conversion function";
  if (MethodDeclAttrs::supports(decl))
    return "method";
  if (ConstructorDeclAttrs::supports(decl))
    return "constructor";
  if (FunctionDeclAttrs::supports(decl))
    return "free function";
  assert(NamedDeclAttrs::supports(decl));
  return "named declaration";
}

static void reportWrongArgumentTypeError(const clang::NamedDecl *decl,
                                         AnnotationKind kind,
                                         const LiteralValue &value) {
  Diagnostics::report(decl, Diagnostics::Kind::AnnotationWrongArgumentTypeError)
      << toString(kind) << toString(value);
}

static void reportWrongNumberOfArgumentsError(const clang::NamedDecl *decl,
                                              AnnotationKind kind) {
  Diagnostics::report(decl,
                      Diagnostics::Kind::AnnotationWrongNumberOfArgumentsError)
      << toString(kind);
}

static void reportInvalidAnnotationError(const clang::NamedDecl *decl,
                                         const Annotation &annotation) {
  Diagnostics::report(decl,
                      Diagnostics::Kind::AnnotationInvalidForDeclKindError)
      << friendlyName(decl) << toString(annotation);
}

namespace {

/// For a given annotation, dispatches to different argument processing
/// functions based on the number of provided arguments.
class ArgumentsDispatcher {
  const clang::NamedDecl *decl;
  AnnotationKind annotation_kind;
  llvm::ArrayRef<LiteralValue> arguments;
  enum class State { Unmatched, Matched, Invalid } state = State::Unmatched;
  const LiteralValue *last_kind_mismatch = nullptr;

  bool checkArgKinds(LiteralValue::Kind kind) {
    auto it = llvm::find_if_not(
        arguments, [&](const LiteralValue &value) { return value.isa(kind); });
    if (it != arguments.end()) {
      last_kind_mismatch = &*it;
      return false;
    }
    return true;
  }

public:
  ArgumentsDispatcher(const clang::NamedDecl *decl,
                      const Annotation &annotation)
      : decl(decl), annotation_kind(annotation.getKind()),
        arguments(annotation.getArguments()) {}

  [[nodiscard]] bool checkMatch() const {
    if (state == State::Unmatched) {
      if (last_kind_mismatch != nullptr) {
        reportWrongArgumentTypeError(decl, annotation_kind,
                                     *last_kind_mismatch);
      } else {
        reportWrongNumberOfArgumentsError(decl, annotation_kind);
      }
    }
    return state == State::Matched;
  }

  template <class NullaryF>
  [[nodiscard]] const ArgumentsDispatcher &onMatch(NullaryF handler) const {
    if (state == State::Matched)
      handler();
    return *this;
  }

  template <class NullaryF> ArgumentsDispatcher &ensure(NullaryF condition) {
    if (state != State::Invalid && !condition())
      state = State::Invalid;
    return *this;
  }

  /// Registers a processing function for calls without arguments.
  template <class NullaryF> ArgumentsDispatcher &nullary(NullaryF process) {
    if (state == State::Unmatched && arguments.empty()) {
      process();
      state = State::Matched;
    }
    return *this;
  }

  /// Registers a processing function for calls with a single argument.
  template <class UnaryF>
  ArgumentsDispatcher &unary(LiteralValue::Kind kind, UnaryF process) {
    if (state == State::Unmatched && arguments.size() == 1 &&
        checkArgKinds(kind)) {
      process(arguments.front());
      state = State::Matched;
    }
    return *this;
  }

  /// Registers a processing function for calls with two arguments.
  template <class BinaryF>
  ArgumentsDispatcher &binary(LiteralValue::Kind kind, BinaryF process) {
    if (state == State::Unmatched && arguments.size() == 2 &&
        checkArgKinds(kind)) {
      auto it = arguments.begin();
      process(*it, *std::next(it));
      state = State::Matched;
    }
    return *this;
  }

  /// Registers a processing function for calls with more than one argument.
  template <class UnaryF>
  ArgumentsDispatcher &variadic(LiteralValue::Kind kind, UnaryF process) {
    if (state == State::Unmatched && !arguments.empty() &&
        checkArgKinds(kind)) {
      for (const LiteralValue &value : arguments)
        process(value);
      state = State::Matched;
    }
    return *this;
  }

  /// Registers a processing function for calls with zero or more arguments.
  /// `transform` is called on each argument and a vector containing the
  /// results is passed to `process`.  `process` should return false, if
  /// the wrong number of arguments were provided.
  template <class UnaryF, class CollectionF>
  ArgumentsDispatcher &variadic(LiteralValue::Kind kind, UnaryF transform,
                                CollectionF process) {
    if (state == State::Unmatched && checkArgKinds(kind)) {
      using ResultT = decltype(transform(LiteralValue()));
      std::vector<ResultT> results;
      results.reserve(arguments.size());
      for (const LiteralValue &value : arguments)
        results.push_back(transform(value));
      if (process(std::move(results)))
        state = State::Matched;
    }
    return *this;
  }
};

} // namespace

bool NamedDeclAttrs::supports(const clang::NamedDecl * /*decl*/) {
  return true;
}

bool genpybind::processAnnotation(const clang::NamedDecl *decl,
                                  const Annotation &annotation,
                                  NamedDeclAttrs &attrs) {
  ArgumentsDispatcher dispatch(decl, annotation);

  switch (annotation.getKind().value()) {
  default:
    return false;

  case AnnotationKind::Hidden:
    return dispatch.nullary([&] { attrs.visible = false; }).checkMatch();

  case AnnotationKind::Visible:
    return dispatch.nullary([&] { attrs.visible = true; })
        .unary(LiteralValue::Kind::Default,
               [&](const LiteralValue &) { attrs.visible = std::nullopt; })
        .unary(LiteralValue::Kind::Boolean,
               [&](const LiteralValue &value) {
                 attrs.visible = value.getBoolean();
               })
        .checkMatch();

  case AnnotationKind::ExposeAs:
    return dispatch
        .unary(LiteralValue::Kind::Default,
               [&](const LiteralValue &) { attrs.spelling.clear(); })
        .unary(LiteralValue::Kind::String,
               [&](const LiteralValue &value) {
                 llvm::StringRef text = value.getString();
                 if (isValidIdentifier(text)) {
                   attrs.spelling = text.str();
                 } else {
                   Diagnostics::report(
                       decl, Diagnostics::Kind::AnnotationInvalidSpellingError)
                       << toString(AnnotationKind::ExposeAs) << text;
                 }
               })
        .checkMatch();

  case AnnotationKind::Module:
    dispatch.ensure([&] { return llvm::isa<clang::NamespaceDecl>(decl); })
        .unary(LiteralValue::Kind::String, [&](const LiteralValue &value) {
          llvm::StringRef text = value.getString();
          if (isValidIdentifier(text)) {
            attrs.spelling = text.str();
          } else {
            Diagnostics::report(
                decl, Diagnostics::Kind::AnnotationInvalidSpellingError)
                << toString(AnnotationKind::Module) << text;
          }
        });
    // Do not check for a match here and return false instead.
    // This way, the annotation is also processed in the overload
    // for namespaces.
    return false;
  }
}

std::string genpybind::getSpelling(const clang::NamedDecl *decl) {
  llvm::SmallString<128> result;
  llvm::raw_svector_ostream os(result);

  // TODO: Use same policy as in expose.cpp
  const clang::PrintingPolicy &policy =
      decl->getASTContext().getPrintingPolicy();

  clang::DeclarationName name = decl->getDeclName();
  name.print(os, policy);

  if (!name.isIdentifier())
    makeValidIdentifier(result);

  if (const auto *tpl_decl =
          llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(decl)) {
    const clang::TemplateArgumentList &args = tpl_decl->getTemplateArgs();
    clang::printTemplateArgumentList(os, args.asArray(), policy);
    makeValidIdentifier(result);
  }

  return result.str().str();
}

bool NamespaceDeclAttrs::supports(const clang::NamedDecl *decl) {
  return llvm::isa<clang::NamespaceDecl>(decl);
}

bool genpybind::processAnnotation(const clang::NamedDecl *decl,
                                  const Annotation &annotation,
                                  NamespaceDeclAttrs &attrs) {
  ArgumentsDispatcher dispatch(decl, annotation);

  switch (annotation.getKind().value()) {
  default:
    return false;

  case AnnotationKind::Module:
    // Note: The spelling is set in the overload for named decls.
    return dispatch.nullary([&]() { attrs.module = true; })
        .unary(LiteralValue::Kind::String,
               [&](const LiteralValue &) { attrs.module = true; })
        .checkMatch();

  case AnnotationKind::OnlyExposeIn:
    return dispatch
        .variadic(LiteralValue::Kind::String,
                  [&](const LiteralValue &value) {
                    attrs.only_expose_in.push_back(value.getString());
                  })
        .checkMatch();
  }
}

bool EnumDeclAttrs::supports(const clang::NamedDecl *decl) {
  return llvm::isa<clang::EnumDecl>(decl);
}

bool genpybind::processAnnotation(const clang::NamedDecl *decl,
                                  const Annotation &annotation,
                                  EnumDeclAttrs &attrs) {
  ArgumentsDispatcher dispatch(decl, annotation);

  switch (annotation.getKind().value()) {
  default:
    return false;

  case AnnotationKind::Arithmetic:
    return dispatch.nullary([&]() { attrs.arithmetic = true; })
        .unary(LiteralValue::Kind::Boolean,
               [&](const LiteralValue &value) {
                 attrs.arithmetic = value.getBoolean();
               })
        .checkMatch();

  case AnnotationKind::ExportValues:
    return dispatch.nullary([&]() { attrs.export_values = true; })
        .unary(LiteralValue::Kind::Boolean,
               [&](const LiteralValue &value) {
                 attrs.export_values = value.getBoolean();
               })
        .checkMatch();
  }
}

bool RecordDeclAttrs::supports(const clang::NamedDecl *decl) {
  return llvm::isa<clang::CXXRecordDecl>(decl);
}

bool genpybind::processAnnotation(const clang::NamedDecl *decl,
                                  const Annotation &annotation,
                                  RecordDeclAttrs &attrs) {
  ArgumentsDispatcher dispatch(decl, annotation);

  const auto *record_decl = llvm::cast<clang::CXXRecordDecl>(decl);

  switch (annotation.getKind().value()) {
  default:
    return false;

  case AnnotationKind::DynamicAttr:
    return dispatch.nullary([&]() { attrs.dynamic_attr = true; })
        .unary(LiteralValue::Kind::Boolean,
               [&](const LiteralValue &value) {
                 attrs.dynamic_attr = value.getBoolean();
               })
        .checkMatch();

  case AnnotationKind::HideBase:
    return dispatch
        .variadic(
            LiteralValue::Kind::String,
            [&](const LiteralValue &value) -> std::string {
              return value.getString();
            },
            [&](const std::vector<std::string> &names) {
              clang::ast_matchers::internal::HasNameMatcher matcher(names);
              bool found_match = false;

              for (const clang::CXXBaseSpecifier &base : record_decl->bases()) {
                const clang::TagDecl *base_decl =
                    base.getType()->getAsTagDecl()->getDefinition();
                if (names.empty() || matcher.matchesNode(*base_decl)) {
                  attrs.hide_base.insert(base_decl);
                  found_match = true;
                }
              }

              if (!found_match) {
                Diagnostics::report(
                    decl,
                    Diagnostics::Kind::AnnotationContainsUnknownBaseTypeWarning)
                    << toString(annotation.getKind().value());
              }
              return true;
            })
        .checkMatch();

  case AnnotationKind::HolderType:
    return dispatch
        .unary(LiteralValue::Kind::String,
               [&](const LiteralValue &value) {
                 attrs.holder_type = value.getString();
               })
        .checkMatch();

  case AnnotationKind::InlineBase:
    return dispatch
        .variadic(
            LiteralValue::Kind::String,
            [&](const LiteralValue &value) -> std::string {
              return value.getString();
            },
            [&](const std::vector<std::string> &names) {
              clang::ast_matchers::internal::HasNameMatcher matcher(names);
              bool found_match = false;
              record_decl->forallBases(
                  [&](const clang::CXXRecordDecl *base_decl) -> bool {
                    base_decl = base_decl->getDefinition();
                    if (names.empty() || matcher.matchesNode(*base_decl)) {
                      attrs.inline_base.insert(base_decl);
                      found_match = true;
                    }
                    return true; // continue visiting other bases
                  });

              if (!found_match) {
                Diagnostics::report(
                    decl,
                    Diagnostics::Kind::AnnotationContainsUnknownBaseTypeWarning)
                    << toString(annotation.getKind().value());
              }
              return true;
            })
        .checkMatch();
  }
}

bool TypedefNameDeclAttrs::supports(const clang::NamedDecl *decl) {
  // TODO: Move to more appropriate place.
  if (const auto *alias_decl = llvm::dyn_cast<clang::TypedefNameDecl>(decl)) {
    const clang::TagDecl *target_decl = aliasTarget(alias_decl);
    if (target_decl == nullptr && hasAnnotations(decl)) {
      Diagnostics::report(decl, Diagnostics::Kind::UnsupportedAliasTargetError);
    }
  }
  return llvm::isa<clang::TypedefNameDecl>(decl);
}

bool genpybind::processAnnotation(const clang::NamedDecl *decl,
                                  const Annotation &annotation,
                                  TypedefNameDeclAttrs &attrs) {
  ArgumentsDispatcher dispatch(decl, annotation);

  auto check_for_conflict = [&] {
    if (attrs.encourage && attrs.expose_here)
      Diagnostics::report(decl, Diagnostics::Kind::ConflictingAnnotationsError)
          << toString(AnnotationKind::Encourage)
          << toString(AnnotationKind::ExposeHere);
  };

  switch (annotation.getKind().value()) {
  default:
    return false;

  case AnnotationKind::Encourage:
    return dispatch.nullary([&] { attrs.encourage = true; })
        .onMatch(check_for_conflict)
        .checkMatch();

  case AnnotationKind::ExposeHere:
    return dispatch.nullary([&] { attrs.expose_here = true; })
        .onMatch(check_for_conflict)
        .checkMatch();
  }
}

void genpybind::propagateAnnotations(AnnotationStorage &annotations,
                                     const clang::TypedefNameDecl *decl) {
  // NOTE: At a later point in time, annotations specific to the declaration
  // kind might be forwarded (e.g. `inline_base`).  They would then need to be
  // checked for validity when first processing the annotations, with any
  // diagnostics being attached to the `TypedefNameDecl` (in order to emit
  // diagnostics in the right order).

  const clang::TagDecl *target_decl = aliasTarget(decl);
  assert(target_decl != nullptr);

  // Ensure that any annotations on the target itself have been processed.
  assert(annotations.has(target_decl));

  const auto attrs = annotations.lookup<NamedDeclAttrs>(decl);
  annotations.update<NamedDeclAttrs>(target_decl, [&](auto &target_attrs) {
    // Always propagate the effective spelling of the type alias, which is the
    // name of its identifier if no explicit `expose_as` annotation has been
    // given.
    target_attrs.spelling =
        attrs.spelling.empty() ? getSpelling(decl) : attrs.spelling;

    // As the `visible` annotatation is implicit if there is at least one other
    // annotation, its computed value also has to be propagated, as it's
    // possible that there is no explicit annotation.
    processAnnotation(
        target_decl,
        Annotation(AnnotationKind::Visible,
                   {attrs.visible.has_value()
                        ? LiteralValue::createBoolean(*attrs.visible)
                        : LiteralValue::createDefault()}),
        target_attrs);
  });
}

const clang::TagDecl *
genpybind::aliasTarget(const clang::TypedefNameDecl *decl) {
  if (const clang::TagDecl *target_decl =
          decl->getUnderlyingType()->getAsTagDecl())
    return target_decl->getDefinition();
  return nullptr;
}

bool FunctionDeclAttrs::supports(const clang::NamedDecl *decl) {
  return llvm::isa<clang::FunctionDecl>(decl) &&
         !llvm::isa<clang::CXXDeductionGuideDecl>(decl) &&
         !llvm::isa<clang::CXXDestructorDecl>(decl);
}

bool genpybind::processAnnotation(const clang::NamedDecl *decl,
                                  const Annotation &annotation,
                                  FunctionDeclAttrs &attrs) {
  ArgumentsDispatcher dispatch(decl, annotation);

  const auto *function_decl = llvm::cast<clang::FunctionDecl>(decl);

  auto make_parameter_indices_lookup = [&](bool add_return, bool add_this) {
    std::vector<llvm::StringRef> parameter_names;
    if (add_return)
      parameter_names.emplace_back("return");
    if (add_this)
      parameter_names.emplace_back("this");
    for (const clang::ParmVarDecl *param : function_decl->parameters())
      parameter_names.push_back(param->getName());

    return [names = std::move(parameter_names)](
               llvm::StringRef name) -> std::optional<unsigned> {
      auto it = llvm::find(names, name);
      if (it == names.end())
        return std::nullopt;

      return static_cast<unsigned>(std::distance(names.begin(), it));
    };
  };

  auto report_invalid_argument = [&](llvm::StringRef arg) {
    Diagnostics::report(
        decl, Diagnostics::Kind::AnnotationInvalidArgumentSpecifierError)
        << toString(annotation.getKind().value()) << arg;
  };

  switch (annotation.getKind().value()) {
  default:
    return false;

  case AnnotationKind::ReturnValuePolicy:
    return dispatch
        .unary(LiteralValue::Kind::String,
               [&](const LiteralValue &value) {
                 attrs.return_value_policy = value.getString();
               })
        .checkMatch();

  case AnnotationKind::KeepAlive:
    return dispatch
        .binary(LiteralValue::Kind::String,
                [&](const LiteralValue &first, const LiteralValue &second) {
                  auto lookup = make_parameter_indices_lookup(
                      true, llvm::isa<clang::CXXMethodDecl>(function_decl));
                  auto first_idx = lookup(first.getString());
                  auto second_idx = lookup(second.getString());
                  if (!first_idx.has_value()) {
                    report_invalid_argument(first.getString());
                    return;
                  }
                  if (!second_idx.has_value()) {
                    report_invalid_argument(second.getString());
                    return;
                  }

                  attrs.keep_alive.emplace_back(*first_idx, *second_idx);
                })
        .checkMatch();

  case AnnotationKind::Noconvert:
    return dispatch
        .variadic(LiteralValue::Kind::String,
                  [&](const LiteralValue &value) {
                    auto lookup = make_parameter_indices_lookup(false, false);
                    auto idx = lookup(value.getString());
                    if (!idx.has_value()) {
                      report_invalid_argument(value.getString());
                      return;
                    }
                    attrs.noconvert.insert(*idx);
                  })
        .checkMatch();

  case AnnotationKind::Required:
    return dispatch
        .variadic(LiteralValue::Kind::String,
                  [&](const LiteralValue &value) {
                    auto lookup = make_parameter_indices_lookup(false, false);
                    auto idx = lookup(value.getString());
                    if (!idx.has_value()) {
                      report_invalid_argument(value.getString());
                      return;
                    }
                    attrs.required.insert(*idx);
                  })
        .checkMatch();
  }
}

bool OperatorDeclAttrs::supports(const clang::NamedDecl *decl) {
  if (const auto *function = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
    return (function->isOverloadedOperator() &&
            function->getOverloadedOperator() != clang::OO_Call);
  }
  return false;
}

bool genpybind::processAnnotation(const clang::NamedDecl * /*decl*/,
                                  const Annotation & /*annotation*/,
                                  OperatorDeclAttrs & /*attrs*/) {
  return false;
}

bool ConversionFunctionDeclAttrs::supports(const clang::NamedDecl *decl) {
  return llvm::isa<clang::CXXConversionDecl>(decl);
}

bool genpybind::processAnnotation(const clang::NamedDecl * /*decl*/,
                                  const Annotation & /*annotation*/,
                                  ConversionFunctionDeclAttrs & /*attrs*/) {
  return false;
}

bool MethodDeclAttrs::supports(const clang::NamedDecl *decl) {
  return llvm::isa<clang::CXXMethodDecl>(decl) &&
         FunctionDeclAttrs::supports(decl) &&
         !llvm::isa<clang::CXXConstructorDecl>(decl) &&
         !OperatorDeclAttrs::supports(decl) &&
         !ConversionFunctionDeclAttrs::supports(decl);
}

bool genpybind::processAnnotation(const clang::NamedDecl *decl,
                                  const Annotation &annotation,
                                  MethodDeclAttrs &attrs) {
  ArgumentsDispatcher dispatch(decl, annotation);

  const auto *function_decl = llvm::cast<clang::FunctionDecl>(decl);

  auto report_invalid_signature = [&]() {
    Diagnostics::report(decl,
                        Diagnostics::Kind::AnnotationIncompatibleSignatureError)
        << friendlyName(decl) << toString(annotation.getKind().value());
  };

  auto collect_identifier = [&](const LiteralValue &value) -> std::string {
    llvm::StringRef identifier = value.getString();
    if (!isValidIdentifier(identifier)) {
      Diagnostics::report(decl,
                          Diagnostics::Kind::AnnotationInvalidSpellingError)
          << toString(annotation.getKind().value()) << identifier;
      return {};
    }
    return identifier.str();
  };

  switch (annotation.getKind().value()) {
  default:
    return false;

  case AnnotationKind::GetterFor:
    return dispatch
        .ensure([&] {
          if (function_decl->getMinRequiredArguments() != 0 ||
              function_decl->getReturnType()->isVoidType()) {
            report_invalid_signature();
            return false;
          }
          return true;
        })
        .variadic(LiteralValue::Kind::String, collect_identifier,
                  [&](std::vector<std::string> identifiers) {
                    if (identifiers.empty())
                      return false;
                    attrs.getter_for.insert(identifiers.begin(),
                                            identifiers.end());
                    return true;
                  })
        .checkMatch();

  case AnnotationKind::SetterFor:
    return dispatch
        .ensure([&] {
          if (function_decl->getNumParams() < 1 ||
              function_decl->getMinRequiredArguments() > 1) {
            report_invalid_signature();
            return false;
          }
          return true;
        })
        .variadic(LiteralValue::Kind::String, collect_identifier,
                  [&](std::vector<std::string> identifiers) {
                    if (identifiers.empty())
                      return false;
                    attrs.setter_for.insert(identifiers.begin(),
                                            identifiers.end());
                    return true;
                  })
        .checkMatch();
  }
}

bool ConstructorDeclAttrs::supports(const clang::NamedDecl *decl) {
  return llvm::isa<clang::CXXConstructorDecl>(decl);
}

bool genpybind::processAnnotation(const clang::NamedDecl *decl,
                                  const Annotation &annotation,
                                  ConstructorDeclAttrs &attrs) {
  ArgumentsDispatcher dispatch(decl, annotation);

  const auto *constructor = llvm::cast<clang::CXXConstructorDecl>(decl);

  switch (annotation.getKind().value()) {
  default:
    return false;

  case AnnotationKind::ImplicitConversion:
    return dispatch
        .ensure([&] {
          if (constructor->getNumParams() != 1 ||
              !constructor->isConvertingConstructor(/*AllowExplicit=*/true)) {
            Diagnostics::report(
                decl, Diagnostics::Kind::AnnotationInvalidForDeclKindError)
                << "non-converting constructor" << toString(annotation);
            return false;
          }
          return true;
        })
        .nullary([&] { attrs.implicit_conversion = true; })
        .unary(LiteralValue::Kind::Boolean,
               [&](const LiteralValue &value) {
                 attrs.implicit_conversion = value.getBoolean();
               })
        .checkMatch();
  }
}

bool FieldOrVarDeclAttrs::supports(const clang::NamedDecl *decl) {
  return llvm::isa<clang::FieldDecl>(decl) || llvm::isa<clang::VarDecl>(decl);
}

bool genpybind::processAnnotation(const clang::NamedDecl *decl,
                                  const Annotation &annotation,
                                  FieldOrVarDeclAttrs &attrs) {
  ArgumentsDispatcher dispatch(decl, annotation);

  // Extract the lambda expression expected for manual bindings from the
  // variable initializer.
  const auto *manual_bindings_expr = [&]() -> const clang::LambdaExpr * {
    const auto *var = llvm::dyn_cast<clang::VarDecl>(decl);
    if (var == nullptr || !var->hasInit())
      return nullptr;

    const clang::Expr *init = var->getInit()->IgnoreImpCasts();
    const auto *lambda = llvm::dyn_cast<clang::LambdaExpr>(init);

    if (lambda == nullptr || !lambda->hasExplicitParameters())
      return nullptr;

    const clang::CXXMethodDecl *method = lambda->getCallOperator();
    if (method->getNumParams() != 1)
      return nullptr;
    const clang::ParmVarDecl *param = method->getParamDecl(0);
    clang::QualType param_type = param->getType();
    if (!param_type->isLValueReferenceType() ||
        param_type.getNonReferenceType()->isUndeducedAutoType())
      return nullptr;

    return lambda;
  }();

  switch (annotation.getKind().value()) {
  default:
    return false;

  // TODO: `readonly` is only supported for fields and static
  // member variables.
  case AnnotationKind::Readonly:
    return dispatch.nullary([&] { attrs.readonly = true; })
        .unary(LiteralValue::Kind::Boolean,
               [&](const LiteralValue &value) {
                 attrs.readonly = value.getBoolean();
               })
        .checkMatch();

  case AnnotationKind::Postamble:
    return dispatch
        .ensure([&] {
          if (manual_bindings_expr == nullptr) {
            return false;
          }
          if (!llvm::isa<clang::TranslationUnitDecl>(decl->getDeclContext())) {
            Diagnostics::report(decl,
                                Diagnostics::Kind::OnlyGlobalScopeAllowedError)
                << toString(annotation.getKind().value());
            return false;
          }
          return true;
        })
        .nullary([&] { attrs.postamble = true; })
        .checkMatch();

  case AnnotationKind::Manual:
    return dispatch.ensure([&] { return manual_bindings_expr != nullptr; })
        .nullary([&] { attrs.manual_bindings = manual_bindings_expr; })
        .checkMatch();
  }
}

void AnnotationStorage::insert(const clang::NamedDecl *decl) {
  assert(decl != nullptr);
  if (LookupContextCollector::shouldSkip(decl)) {
    Diagnostics::report(decl, Diagnostics::Kind::InvalidAssumptionWarning)
        << "annotations are only parsed during initial pass";
    decl->dump();
    return;
  }

  if (has(decl))
    return;

  std::vector<std::function<bool(const Annotation &)>> handlers;
  std::apply(
      [&](auto &...map) {
        ((llvm::remove_cvref_t<decltype(map)>::mapped_type::supports(decl)
              ? handlers.push_back(
                    [decl, &attrs = map[decl]](const Annotation &annotation) {
                      return processAnnotation(decl, annotation, attrs);
                    })
              : void()),
         ...);
      },
      attrs_by_decl);

  // Non-namespace named decls that have at least one annotation
  // are visible by default.  This can be overruled by an explicit
  // `visible(default)`, `visible(false)` or `hidden` annotation.
  if (!llvm::isa<clang::NamespaceDecl>(decl) &&
      hasAnnotations(decl, /*allow_empty=*/false)) {
    std::get<Map<NamedDeclAttrs>>(attrs_by_decl)[decl].visible = true;
  }

  clang::DiagnosticsEngine &diag = decl->getASTContext().getDiagnostics();
  const Parser::Annotations annotations = parseAnnotations(decl);
  for (const Annotation &annotation : annotations) {
    clang::DiagnosticErrorTrap trap{diag};
    bool handled = llvm::any_of(
        handlers, [&](const auto &handler) { return handler(annotation); });
    if (!trap.hasErrorOccurred() && !handled) {
      reportInvalidAnnotationError(decl, annotation);
    }
  }
}

bool AnnotationStorage::equal(const clang::NamedDecl *left,
                              const clang::NamedDecl *right) const {
  return std::apply(
      [&](auto... x) {
        return (
            (this->get<decltype(x)>(left) == this->get<decltype(x)>(right)) &&
            ...);
      },
      AttrTypes{});
}
