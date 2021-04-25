#include "genpybind/annotated_decl.h"

#include "genpybind/annotations/literal_value.h"
#include "genpybind/annotations/parser.h"
#include "genpybind/diagnostics.h"
#include "genpybind/lookup_context_collector.h"
#include "genpybind/string_utils.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Attr.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/DeclarationName.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/CharInfo.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/None.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Compiler.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <string>
#include <type_traits>
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
        ::llvm::StringRef annotation_text = attr->getAnnotation();
        if (!annotation_text.startswith(k_lozenge))
          return false;
        if (allow_empty)
          return true;
        return !annotation_text.drop_front(k_lozenge.size()).ltrim().empty();
      });
}

llvm::SmallVector<llvm::StringRef, 1>
genpybind::getAnnotationStrings(const clang::Decl *decl) {
  llvm::SmallVector<llvm::StringRef, 1> result;
  for (const auto *attr : decl->specific_attrs<::clang::AnnotateAttr>()) {
    ::llvm::StringRef annotation_text = attr->getAnnotation();
    if (!annotation_text.consume_front(k_lozenge))
      continue;
    result.push_back(annotation_text);
  }
  return result;
}

static Parser::Annotations parseAnnotations(const ::clang::Decl *decl) {
  Parser::Annotations annotations;

  for (const auto *attr : decl->specific_attrs<::clang::AnnotateAttr>()) {
    ::llvm::StringRef annotation_text = attr->getAnnotation();
    if (!annotation_text.consume_front(k_lozenge))
      continue;
    handleAllErrors(Parser::parseAnnotations(annotation_text, annotations),
                    [attr, decl](const Parser::Error &error) {
                      ::clang::ASTContext &context = decl->getASTContext();
                      const ::clang::SourceLocation loc =
                          context.getSourceManager().getExpansionLoc(
                              attr->getLocation());
                      error.report(loc, context.getDiagnostics());
                    });
  }

  return annotations;
}

static llvm::StringRef friendlyName(AnnotatedDecl::Kind kind) {
  using Kind = AnnotatedDecl::Kind;
  switch (kind) {
  case Kind::Named:
    return "named declaration";
  case Kind::Namespace:
    return "namespace";
  case Kind::Enum:
    return "enum";
  case Kind::Class:
    return "class";
  case Kind::TypeAlias:
    return "type alias";
  case Kind::Variable:
    return "variable";
  case Kind::Operator:
    return "operator";
  case Kind::Function:
    return "free function";
  case Kind::ConversionFunction:
    return "conversion function";
  case Kind::Method:
    return "method";
  case Kind::Constructor:
    return "constructor";
  default:
    llvm_unreachable("Unexpected annotated decl kind.");
  }
}

static void reportWrongArgumentTypeError(const clang::Decl *decl,
                                         AnnotationKind kind,
                                         const LiteralValue &value) {
  Diagnostics::report(decl, Diagnostics::Kind::AnnotationWrongArgumentTypeError)
      << toString(kind) << toString(value);
}

static void reportWrongNumberOfArgumentsError(const clang::Decl *decl,
                                              AnnotationKind kind) {
  Diagnostics::report(decl,
                      Diagnostics::Kind::AnnotationWrongNumberOfArgumentsError)
      << toString(kind);
}

static void reportInvalidAnnotationError(const clang::Decl *decl,
                                         AnnotatedDecl::Kind kind,
                                         const Annotation &annotation) {
  Diagnostics::report(decl,
                      Diagnostics::Kind::AnnotationInvalidForDeclKindError)
      << friendlyName(kind) << toString(annotation);
}

namespace {

/// For a given annotation, dispatches to different argument processing
/// functions based on the number of provided arguments.
class ArgumentsDispatcher {
  const clang::Decl *decl;
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
  ArgumentsDispatcher(const clang::Decl *decl, const Annotation &annotation)
      : decl(decl), annotation_kind(annotation.getKind()),
        arguments(annotation.getArguments()) {}

  LLVM_NODISCARD bool checkMatch() const {
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
  const ArgumentsDispatcher &onMatch(NullaryF handler) const {
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

std::unique_ptr<AnnotatedDecl>
AnnotatedDecl::create(const clang::NamedDecl *decl) {
  assert(decl != nullptr);
  auto result = [decl]() -> std::unique_ptr<AnnotatedNamedDecl> {
    if (const auto *alias_decl = llvm::dyn_cast<clang::TypedefNameDecl>(decl)) {
      // TODO: Move to more appropriate place.
      const clang::TagDecl *target_decl =
          AnnotatedTypedefNameDecl::aliasTarget(alias_decl);
      if (target_decl != nullptr) {
        return std::make_unique<AnnotatedTypedefNameDecl>();
      } else if (hasAnnotations(decl)) {
        Diagnostics::report(decl,
                            Diagnostics::Kind::UnsupportedAliasTargetError);
      }
    }
    if (llvm::isa<clang::NamespaceDecl>(decl))
      return std::make_unique<AnnotatedNamespaceDecl>();
    if (llvm::isa<clang::EnumDecl>(decl))
      return std::make_unique<AnnotatedEnumDecl>();
    if (llvm::isa<clang::CXXRecordDecl>(decl))
      return std::make_unique<AnnotatedRecordDecl>();
    if (llvm::isa<clang::FieldDecl>(decl) || llvm::isa<clang::VarDecl>(decl))
      return std::make_unique<AnnotatedFieldOrVarDecl>();

    if (llvm::isa<clang::CXXDeductionGuideDecl>(decl) ||
        llvm::isa<clang::CXXDestructorDecl>(decl)) {
      // Fall through to generic AnnotatedNamedDecl case.
    } else if (llvm::isa<clang::CXXConversionDecl>(decl)) {
      return std::make_unique<AnnotatedConversionFunctionDecl>();
    } else if (llvm::isa<clang::CXXConstructorDecl>(decl)) {
      return std::make_unique<AnnotatedConstructorDecl>();
    } else if (const auto *function =
                   llvm::dyn_cast<clang::FunctionDecl>(decl)) {
      if (function->isOverloadedOperator() &&
          function->getOverloadedOperator() != clang::OO_Call)
        return std::make_unique<AnnotatedOperatorDecl>();

      if (llvm::isa<clang::CXXMethodDecl>(decl)) {
        // The extra annotations only apply to methods, not to derived classes
        // such as constructors.  Therefore `AnnotatedConstructorDecl` directly
        // derives from `AnnotatedFunctionDecl`.
        assert(decl->getKind() == clang::Decl::Kind::CXXMethod);
        return std::make_unique<AnnotatedMethodDecl>();
      }

      return std::make_unique<AnnotatedFunctionDecl>();
    }

    return std::make_unique<AnnotatedNamedDecl>();
  }();

  // Non-namespace named decls that have at least one annotation
  // are visible by default.  This can be overruled by an explicit
  // `visible(default)`, `visible(false)` or `hidden` annotation.
  if (result != nullptr && !llvm::isa<clang::NamespaceDecl>(decl) &&
      hasAnnotations(decl, /*allow_empty=*/false))
    result->visible = true;

  return result;
}

void AnnotatedDecl::processAnnotations(const clang::Decl *decl) {
  if (LookupContextCollector::shouldSkip(decl)) {
    Diagnostics::report(decl, Diagnostics::Kind::InvalidAssumptionWarning)
        << "annotations are only parsed during initial pass";
    decl->dump();
    return;
  }
  const Parser::Annotations annotations = parseAnnotations(decl);
  for (const Annotation &annotation : annotations) {
    processAnnotation(decl, annotation);
  }
}

bool AnnotatedNamedDecl::processAnnotation(const clang::Decl *decl,
                                           const Annotation &annotation) {
  ArgumentsDispatcher dispatch(decl, annotation);

  switch (annotation.getKind().value()) {
  default:
    reportInvalidAnnotationError(decl, getKind(), annotation);
    return false;

  case AnnotationKind::Hidden:
    return dispatch.nullary([&] { visible = false; }).checkMatch();

  case AnnotationKind::Visible:
    return dispatch.nullary([&] { visible = true; })
        .unary(LiteralValue::Kind::Default,
               [&](const LiteralValue &) { visible = llvm::None; })
        .unary(LiteralValue::Kind::Boolean,
               [&](const LiteralValue &value) { visible = value.getBoolean(); })
        .checkMatch();

  case AnnotationKind::ExposeAs:
    return dispatch
        .unary(LiteralValue::Kind::Default,
               [&](const LiteralValue &) { spelling.clear(); })
        .unary(LiteralValue::Kind::String,
               [&](const LiteralValue &value) {
                 llvm::StringRef text = value.getString();
                 if (clang::isValidIdentifier(text)) {
                   spelling = text.str();
                 } else {
                   Diagnostics::report(
                       decl, Diagnostics::Kind::AnnotationInvalidSpellingError)
                       << toString(AnnotationKind::ExposeAs) << text;
                 }
               })
        .checkMatch();
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

bool AnnotatedNamespaceDecl::processAnnotation(const clang::Decl *decl,
                                               const Annotation &annotation) {
  ArgumentsDispatcher dispatch(decl, annotation);

  switch (annotation.getKind().value()) {
  default:
    return AnnotatedNamedDecl::processAnnotation(decl, annotation);

  case AnnotationKind::Module:
    return dispatch.nullary([&]() { module = true; })
        .unary(LiteralValue::Kind::String,
               [&](const LiteralValue &value) {
                 llvm::StringRef text = value.getString();
                 if (clang::isValidIdentifier(text)) {
                   module = true;
                   spelling = text.str();
                 } else {
                   Diagnostics::report(
                       decl, Diagnostics::Kind::AnnotationInvalidSpellingError)
                       << toString(AnnotationKind::Module) << text;
                 }
               })
        .checkMatch();

  case AnnotationKind::OnlyExposeIn:
    return dispatch
        .variadic(LiteralValue::Kind::String,
                  [&](const LiteralValue &value) {
                    only_expose_in.push_back(value.getString());
                  })
        .checkMatch();
  }
}

bool AnnotatedEnumDecl::processAnnotation(const clang::Decl *decl,
                                          const Annotation &annotation) {
  ArgumentsDispatcher dispatch(decl, annotation);

  switch (annotation.getKind().value()) {
  default:
    return AnnotatedNamedDecl::processAnnotation(decl, annotation);

  case AnnotationKind::Arithmetic:
    return dispatch
        .nullary([&]() { arithmetic = true; })
        // TODO: "default"?
        .unary(
            LiteralValue::Kind::Boolean,
            [&](const LiteralValue &value) { arithmetic = value.getBoolean(); })
        .checkMatch();

  case AnnotationKind::ExportValues:
    return dispatch
        .nullary([&]() { export_values = true; })
        // TODO: Remove "default"?
        .unary(LiteralValue::Kind::Default,
               [&](const LiteralValue &) { export_values = llvm::None; })
        .unary(LiteralValue::Kind::Boolean,
               [&](const LiteralValue &value) {
                 export_values = value.getBoolean();
               })
        .checkMatch();
  }
}

bool AnnotatedRecordDecl::processAnnotation(const clang::Decl *decl,
                                            const Annotation &annotation) {
  ArgumentsDispatcher dispatch(decl, annotation);

  const auto *record_decl = llvm::cast<clang::CXXRecordDecl>(decl);

  switch (annotation.getKind().value()) {
  default:
    return AnnotatedNamedDecl::processAnnotation(decl, annotation);

  case AnnotationKind::DynamicAttr:
    return dispatch
        .nullary([&]() { dynamic_attr = true; })
        // TODO: "default"?
        .unary(LiteralValue::Kind::Boolean,
               [&](const LiteralValue &value) {
                 dynamic_attr = value.getBoolean();
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
                  hide_base.insert(base_decl);
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
        .unary(
            LiteralValue::Kind::String,
            [&](const LiteralValue &value) { holder_type = value.getString(); })
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
                      inline_base.insert(base_decl);
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

bool AnnotatedTypedefNameDecl::processAnnotation(const clang::Decl *decl,
                                                 const Annotation &annotation) {
  ArgumentsDispatcher dispatch(decl, annotation);

  auto check_for_conflict = [&] {
    if (encourage && expose_here)
      Diagnostics::report(decl, Diagnostics::Kind::ConflictingAnnotationsError)
          << toString(AnnotationKind::Encourage)
          << toString(AnnotationKind::ExposeHere);
  };

  switch (annotation.getKind().value()) {
  default:
    return AnnotatedNamedDecl::processAnnotation(decl, annotation);

  case AnnotationKind::Encourage:
    return dispatch.nullary([&] { encourage = true; })
        .onMatch(check_for_conflict)
        .checkMatch();

  case AnnotationKind::ExposeHere:
    return dispatch.nullary([&] { expose_here = true; })
        .onMatch(check_for_conflict)
        .checkMatch();
  }
}

void AnnotatedTypedefNameDecl::propagateAnnotations(
    const clang::TypedefNameDecl *decl, AnnotatedNamedDecl *other) const {
  // NOTE: At a later point in time, annotations specific to the declaration
  // kind might be forwarded (e.g. `inline_base`).  They would then need to be
  // checked for validity in `processAnnotations`, with any diagnostics being
  // attached to the `TypedefNameDecl` (in order to emit diagnostics in the
  // right order).

  const clang::TagDecl *target_decl = aliasTarget(decl);
  assert(target_decl != nullptr);

  // Always propagate the effective spelling of the type alias, which is the
  // name of its identifier if no explicit `expose_as` annotation has been
  // given.  If there is one, it will be processed twice, but this is benign.
  other->processAnnotation(
      target_decl,
      Annotation(AnnotationKind::ExposeAs,
                 {LiteralValue::createString(
                     spelling.empty() ? getSpelling(decl) : spelling)}));
  // As the `visible` annotatation is implicit if there is at least one other
  // annotation, its computed value also has to be propagated, as it's possible
  // that there is no explicit annotation.
  other->processAnnotation(
      target_decl,
      Annotation(AnnotationKind::Visible,
                 {visible.hasValue() ? LiteralValue::createBoolean(*visible)
                                     : LiteralValue::createDefault()}));
}

const clang::TagDecl *
AnnotatedTypedefNameDecl::aliasTarget(const clang::TypedefNameDecl *decl) {
  if (const clang::TagDecl *target_decl =
          decl->getUnderlyingType()->getAsTagDecl())
    return target_decl->getDefinition();
  return nullptr;
}

bool AnnotatedFunctionDecl::processAnnotation(const clang::Decl *decl,
                                              const Annotation &annotation) {
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
               llvm::StringRef name) -> llvm::Optional<unsigned> {
      auto it = llvm::find(names, name);
      if (it == names.end())
        return llvm::None;

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
    return AnnotatedNamedDecl::processAnnotation(decl, annotation);

  case AnnotationKind::ReturnValuePolicy:
    return dispatch
        .unary(LiteralValue::Kind::String,
               [&](const LiteralValue &value) {
                 return_value_policy = value.getString();
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
                  if (!first_idx.hasValue()) {
                    report_invalid_argument(first.getString());
                    return;
                  }
                  if (!second_idx.hasValue()) {
                    report_invalid_argument(second.getString());
                    return;
                  }

                  keep_alive.emplace_back(*first_idx, *second_idx);
                })
        .checkMatch();

  case AnnotationKind::Noconvert:
    return dispatch
        .variadic(LiteralValue::Kind::String,
                  [&](const LiteralValue &value) {
                    auto lookup = make_parameter_indices_lookup(false, false);
                    auto idx = lookup(value.getString());
                    if (!idx.hasValue()) {
                      report_invalid_argument(value.getString());
                      return;
                    }
                    noconvert.insert(*idx);
                  })
        .checkMatch();

  case AnnotationKind::Required:
    return dispatch
        .variadic(LiteralValue::Kind::String,
                  [&](const LiteralValue &value) {
                    auto lookup = make_parameter_indices_lookup(false, false);
                    auto idx = lookup(value.getString());
                    if (!idx.hasValue()) {
                      report_invalid_argument(value.getString());
                      return;
                    }
                    required.insert(*idx);
                  })
        .checkMatch();
  }
}

bool AnnotatedMethodDecl::processAnnotation(const clang::Decl *decl,
                                            const Annotation &annotation) {
  ArgumentsDispatcher dispatch(decl, annotation);

  const auto *function_decl = llvm::cast<clang::FunctionDecl>(decl);

  auto report_invalid_signature = [&]() {
    Diagnostics::report(decl,
                        Diagnostics::Kind::AnnotationIncompatibleSignatureError)
        << friendlyName(getKind()) << toString(annotation.getKind().value());
  };

  auto collect_identifier = [&](const LiteralValue &value) -> std::string {
    llvm::StringRef identifier = value.getString();
    if (!clang::isValidIdentifier(identifier)) {
      Diagnostics::report(decl,
                          Diagnostics::Kind::AnnotationInvalidSpellingError)
          << toString(annotation.getKind().value()) << identifier;
      return {};
    }
    return identifier.str();
  };

  switch (annotation.getKind().value()) {
  default:
    return AnnotatedFunctionDecl::processAnnotation(decl, annotation);

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
                    getter_for.insert(identifiers.begin(), identifiers.end());
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
                    setter_for.insert(identifiers.begin(), identifiers.end());
                    return true;
                  })
        .checkMatch();
  }
}

bool AnnotatedConstructorDecl::processAnnotation(const clang::Decl *decl,
                                                 const Annotation &annotation) {
  ArgumentsDispatcher dispatch(decl, annotation);

  const auto *constructor = llvm::cast<clang::CXXConstructorDecl>(decl);

  switch (annotation.getKind().value()) {
  default:
    return AnnotatedFunctionDecl::processAnnotation(decl, annotation);

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
        .nullary([&] { implicit_conversion = true; })
        .unary(LiteralValue::Kind::Boolean,
               [&](const LiteralValue &value) {
                 implicit_conversion = value.getBoolean();
               })
        .checkMatch();
  }
}

bool AnnotatedFieldOrVarDecl::processAnnotation(const clang::Decl *decl,
                                                const Annotation &annotation) {
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
    return AnnotatedNamedDecl::processAnnotation(decl, annotation);

  // TODO: `readonly` is only supported for fields and static
  // member variables.
  case AnnotationKind::Readonly:
    return dispatch.nullary([&] { readonly = true; })
        .unary(
            LiteralValue::Kind::Boolean,
            [&](const LiteralValue &value) { readonly = value.getBoolean(); })
        .checkMatch();

  case AnnotationKind::Postamble:
    return dispatch
        .ensure([&] {
          if (manual_bindings_expr == nullptr) {
            reportInvalidAnnotationError(decl, getKind(), annotation);
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
        .nullary([&] { postamble = true; })
        .checkMatch();

  case AnnotationKind::Manual:
    return dispatch
        .ensure([&] {
          if (manual_bindings_expr == nullptr) {
            reportInvalidAnnotationError(decl, getKind(), annotation);
            return false;
          }
          return true;
        })
        .nullary([&] { manual_bindings = manual_bindings_expr; })
        .checkMatch();
  }
}

AnnotatedDecl *AnnotationStorage::getOrInsert(const clang::Decl *declaration) {
  if (const auto *named_decl =
          llvm::dyn_cast_or_null<clang::NamedDecl>(declaration)) {
    auto result =
        annotations.try_emplace(named_decl, AnnotatedDecl::create(named_decl));
    AnnotatedDecl *annotated_decl = result.first->getSecond().get();
    assert(annotated_decl != nullptr);
    if (result.second)
      annotated_decl->processAnnotations(named_decl);
    return annotated_decl;
  }
  return nullptr;
}

const AnnotatedDecl *
AnnotationStorage::get(const clang::Decl *declaration) const {
  if (const auto *named_decl =
          llvm::dyn_cast_or_null<clang::NamedDecl>(declaration)) {
    auto it = annotations.find(named_decl);
    if (it != annotations.end())
      return it->second.get();
  }
  return nullptr;
}
