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

namespace {

class ArgumentsConsumer {
  llvm::ArrayRef<LiteralValue> remaining;

public:
  ArgumentsConsumer(llvm::ArrayRef<LiteralValue> remaining)
      : remaining(remaining) {}

  llvm::Optional<LiteralValue> take() {
    if (!remaining.empty()) {
      LiteralValue value = remaining.front();
      remaining = remaining.drop_front();
      return value;
    }
    return llvm::None;
  }

  template <LiteralValue::Kind kind> llvm::Optional<LiteralValue> take() {
    if (!remaining.empty() && remaining.front().is<kind>()) {
      return take();
    }
    return llvm::None;
  }

  std::size_t size() const { return remaining.size(); }
  bool empty() const { return remaining.empty(); }
  operator bool() const { return !remaining.empty(); }
};

} // namespace

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

std::unique_ptr<AnnotatedDecl>
AnnotatedDecl::create(const clang::NamedDecl *decl) {
  assert(decl != nullptr);
  auto result = [decl]() -> std::unique_ptr<AnnotatedDecl> {
    if (llvm::isa<clang::TypedefNameDecl>(decl))
      return std::make_unique<AnnotatedTypedefNameDecl>();
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
    result->processAnnotation(decl,
                              Annotation(AnnotationKind::Visible,
                                         {LiteralValue::createBoolean(true)}));

  // TODO: Move to more appropriate place.
  if (const auto *alias_decl = llvm::dyn_cast<clang::TypedefNameDecl>(decl)) {
    const clang::TagDecl *target_decl =
        alias_decl->getUnderlyingType()->getAsTagDecl();
    if (hasAnnotations(decl) && target_decl == nullptr)
      Diagnostics::report(decl, Diagnostics::Kind::UnsupportedAliasTargetError);
  }

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
    if (!processAnnotation(decl, annotation)) {
      Diagnostics::report(decl,
                          Diagnostics::Kind::AnnotationInvalidForDeclKindError)
          << friendlyName(getKind()) << toString(annotation);
    }
  }
}

bool AnnotatedNamedDecl::processAnnotation(const clang::Decl *decl,
                                           const Annotation &annotation) {
  ArgumentsConsumer arguments(annotation.getArguments());
  switch (annotation.getKind().value()) {
  case AnnotationKind::Hidden:
    visible = false;
    break;
  case AnnotationKind::Visible:
    if (arguments.empty()) {
      visible = true;
    } else if (arguments.take<LiteralValue::Kind::Default>()) {
      visible = llvm::None;
    } else if (auto value = arguments.take<LiteralValue::Kind::Boolean>()) {
      visible = value->getBoolean();
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(decl, annotation.getKind(), *value);
    }
    break;
  case AnnotationKind::ExposeAs:
    if (arguments.empty()) {
      reportWrongNumberOfArgumentsError(decl, annotation.getKind());
    } else if (arguments.take<LiteralValue::Kind::Default>()) {
      spelling.clear();
    } else if (auto value = arguments.take<LiteralValue::Kind::String>()) {
      llvm::StringRef text = value->getString();
      if (clang::isValidIdentifier(text)) {
        spelling = text.str();
      } else {
        Diagnostics::report(decl,
                            Diagnostics::Kind::AnnotationInvalidSpellingError)
            << toString(AnnotationKind::ExposeAs) << text;
      }
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(decl, annotation.getKind(), *value);
    }
    break;
  default:
    return false;
  }
  if (arguments)
    reportWrongNumberOfArgumentsError(decl, annotation.getKind());
  return true;
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
  if (AnnotatedNamedDecl::processAnnotation(decl, annotation))
    return true;

  ArgumentsConsumer arguments(annotation.getArguments());
  switch (annotation.getKind().value()) {
  case AnnotationKind::Module:
    if (arguments.empty()) {
      module = true;
    } else if (auto value = arguments.take<LiteralValue::Kind::String>()) {
      module = true;
      AnnotatedNamedDecl::processAnnotation(
          decl, Annotation(AnnotationKind::ExposeAs, {*value}));
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(decl, annotation.getKind(), *value);
    }
    break;
  case AnnotationKind::OnlyExposeIn:
    if (arguments.empty())
      reportWrongNumberOfArgumentsError(decl, annotation.getKind());
    while (auto value = arguments.take<LiteralValue::Kind::String>())
      only_expose_in.push_back(value->getString());
    if (auto value = arguments.take())
      reportWrongArgumentTypeError(decl, annotation.getKind(), *value);
    break;
  default:
    return false;
  }
  if (arguments)
    reportWrongNumberOfArgumentsError(decl, annotation.getKind());
  return true;
}

bool AnnotatedEnumDecl::processAnnotation(const clang::Decl *decl,
                                          const Annotation &annotation) {
  if (AnnotatedNamedDecl::processAnnotation(decl, annotation))
    return true;

  ArgumentsConsumer arguments(annotation.getArguments());
  switch (annotation.getKind().value()) {
  case AnnotationKind::Arithmetic:
    if (arguments.empty()) {
      arithmetic = true;
    } else if (auto value = arguments.take<LiteralValue::Kind::Boolean>()) {
      arithmetic = value->getBoolean();
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(decl, annotation.getKind(), *value);
    }
    break;
  case AnnotationKind::ExportValues:
    if (arguments.empty()) {
      export_values = true;
    } else if (arguments.take<LiteralValue::Kind::Default>()) {
      export_values = llvm::None;
    } else if (auto value = arguments.take<LiteralValue::Kind::Boolean>()) {
      export_values = value->getBoolean();
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(decl, annotation.getKind(), *value);
    }
    break;
  default:
    return false;
  }
  if (arguments)
    reportWrongNumberOfArgumentsError(decl, annotation.getKind());
  return true;
}

bool AnnotatedRecordDecl::processAnnotation(const clang::Decl *decl,
                                            const Annotation &annotation) {
  if (AnnotatedNamedDecl::processAnnotation(decl, annotation))
    return true;

  const auto *record_decl = llvm::cast<clang::CXXRecordDecl>(decl);
  ArgumentsConsumer arguments(annotation.getArguments());
  switch (annotation.getKind().value()) {
  case AnnotationKind::DynamicAttr:
    if (arguments.empty()) {
      dynamic_attr = true;
    } else if (auto value = arguments.take<LiteralValue::Kind::Boolean>()) {
      dynamic_attr = value->getBoolean();
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(decl, annotation.getKind(), *value);
    }
    break;
  case AnnotationKind::HideBase: {
    std::vector<std::string> names;
    while (auto value = arguments.take<LiteralValue::Kind::String>())
      names.push_back(value->getString());
    if (auto value = arguments.take())
      reportWrongArgumentTypeError(decl, annotation.getKind(), *value);

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
          decl, Diagnostics::Kind::AnnotationContainsUnknownBaseTypeWarning)
          << toString(annotation.getKind());
    }
    break;
  }
  case AnnotationKind::HolderType:
    if (auto value = arguments.take<LiteralValue::Kind::String>()) {
      holder_type = value->getString();
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(decl, annotation.getKind(), *value);
    }
    break;
  case AnnotationKind::InlineBase: {
    std::vector<std::string> names;
    while (auto value = arguments.take<LiteralValue::Kind::String>())
      names.push_back(value->getString());
    if (auto value = arguments.take())
      reportWrongArgumentTypeError(decl, annotation.getKind(), *value);

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
          decl, Diagnostics::Kind::AnnotationContainsUnknownBaseTypeWarning)
          << toString(annotation.getKind());
    }
    break;
  }
  default:
    return false;
  }
  if (arguments)
    reportWrongNumberOfArgumentsError(decl, annotation.getKind());
  return true;
}

bool AnnotatedTypedefNameDecl::processAnnotation(const clang::Decl *decl,
                                                 const Annotation &annotation) {
  if (AnnotatedNamedDecl::processAnnotation(decl, annotation)) {
    // Keep track of general `NamedDecl` annotations in order to forward
    // them if this is an "expose_here" type alias.
    // NOTE: At a later point in time, annotations specific to the declaration
    // kind might be forwarded (e.g. `inline_base`).  They would then need to be
    // checked for validity here, with any diagnostics being attached to the
    // `TypedefNameDecl` (in order to emit diagnostics in the right order).
    annotations_to_propagate.push_back(annotation);
    return true;
  }

  ArgumentsConsumer arguments(annotation.getArguments());
  switch (annotation.getKind().value()) {
  case AnnotationKind::Encourage:
    encourage = true;
    break;
  case AnnotationKind::ExposeHere:
    expose_here = true;
    break;
  default:
    return false;
  }
  if (arguments)
    reportWrongNumberOfArgumentsError(decl, annotation.getKind());

  if (encourage && expose_here)
    Diagnostics::report(decl, Diagnostics::Kind::ConflictingAnnotationsError)
        << toString(AnnotationKind::Encourage)
        << toString(AnnotationKind::ExposeHere);

  return true;
}

void AnnotatedTypedefNameDecl::propagateAnnotations(
    const clang::Decl *decl,
    AnnotatedDecl &other) const {
  // FIXME: change parameter type
  const auto *const named_decl = llvm::cast<clang::NamedDecl>(decl);
  // Always propagate the effective spelling of the type alias, which is the
  // name of its identifier if no explicit `expose_as` annotation has been
  // given.  If there is one, it will be processed twice, but this is benign.
  other.processAnnotation(
      decl,
      Annotation(AnnotationKind::ExposeAs,
                 {LiteralValue::createString(
                     spelling.empty() ? getSpelling(named_decl) : spelling)}));
  // As the `visible` annotatation is implicit if there is at least one other
  // annotation, its computed value also has to be propagated, as it's possible
  // that there is no explicit annotation.
  other.processAnnotation(
      decl,
      Annotation(AnnotationKind::Visible,
                 {visible.hasValue() ? LiteralValue::createBoolean(*visible)
                                     : LiteralValue::createDefault()}));
  for (const Annotation &annotation : annotations_to_propagate) {
    bool result = other.processAnnotation(decl, annotation);
    // So far, only annotations that are valid for all `NamedDecl`s are
    // propagated.  If additional annotations are forwarded in the future,
    // they need to be checked for validity when processing the
    // `TypedefNameDecl`'s annotations (see note above).
    assert(result && "invalid annotation has been propagated");
    static_cast<void>(result);
  }
}

bool AnnotatedFunctionDecl::processAnnotation(const clang::Decl *decl,
                                              const Annotation &annotation) {
  if (AnnotatedNamedDecl::processAnnotation(decl, annotation))
    return true;

  ArgumentsConsumer arguments(annotation.getArguments());
  std::vector<llvm::StringRef> parameter_names;
  switch (annotation.getKind().value()) {
  case AnnotationKind::KeepAlive:
    if (arguments.size() != 2) {
      reportWrongNumberOfArgumentsError(decl, annotation.getKind());
      return true;
    }
    parameter_names.emplace_back("return");
    if (llvm::isa<clang::CXXMethodDecl>(decl))
      parameter_names.emplace_back("this");
    break;
  case AnnotationKind::Noconvert:
  case AnnotationKind::Required:
    if (arguments.empty()) {
      reportWrongNumberOfArgumentsError(decl, annotation.getKind());
      return true;
    }
    break;
  case AnnotationKind::ReturnValuePolicy: {
    if (arguments.size() != 1) {
      reportWrongNumberOfArgumentsError(decl, annotation.getKind());
    } else if (auto value = arguments.take<LiteralValue::Kind::String>()) {
      return_value_policy = value->getString();
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(decl, annotation.getKind(), *value);
    }
    return true;
  }
  default:
    return false;
  }

  const auto *function_decl = llvm::cast<clang::FunctionDecl>(decl);
  for (const clang::ParmVarDecl *param : function_decl->parameters()) {
    parameter_names.push_back(param->getName());
  }

  bool success = true;
  llvm::SmallVector<unsigned, 2> parameter_indices;
  while (auto value = arguments.take<LiteralValue::Kind::String>()) {
    auto it = llvm::find(parameter_names, value->getString());
    if (it == parameter_names.end()) {
      Diagnostics::report(
          decl, Diagnostics::Kind::AnnotationInvalidArgumentSpecifierError)
          << toString(annotation.getKind()) << value->getString();
      success = false;
      continue;
    }
    parameter_indices.push_back(
        static_cast<unsigned>(std::distance(parameter_names.begin(), it)));
  }
  if (auto value = arguments.take()) {
    reportWrongArgumentTypeError(decl, annotation.getKind(), *value);
    success = false;
  }

  if (success) {
    switch (annotation.getKind().value()) {
    case AnnotationKind::KeepAlive:
      assert(parameter_indices.size() == 2);
      keep_alive.emplace_back(parameter_indices[0], parameter_indices[1]);
      break;
    case AnnotationKind::Noconvert:
      noconvert.insert(parameter_indices.begin(), parameter_indices.end());
      break;
    case AnnotationKind::Required:
      required.insert(parameter_indices.begin(), parameter_indices.end());
      break;
    default:
      llvm_unreachable("Unexpected annotation kind.");
    }
  }

  return true;
}

bool AnnotatedMethodDecl::processAnnotation(const clang::Decl *decl,
                                            const Annotation &annotation) {
  if (AnnotatedFunctionDecl::processAnnotation(decl, annotation))
    return true;

  auto report_invalid_signature = [&]() {
    Diagnostics::report(decl,
                        Diagnostics::Kind::AnnotationIncompatibleSignatureError)
        << friendlyName(getKind()) << toString(annotation.getKind());
  };

  const auto *function_decl = llvm::cast<clang::FunctionDecl>(decl);

  clang::QualType return_type = function_decl->getReturnType();
  switch (annotation.getKind().value()) {
  case AnnotationKind::GetterFor:
    if (function_decl->getMinRequiredArguments() != 0 || return_type->isVoidType())
      report_invalid_signature();
    break;
  case AnnotationKind::SetterFor:
    if (function_decl->getNumParams() < 1 || function_decl->getMinRequiredArguments() > 1)
      report_invalid_signature();
    break;
  default:
    return false;
  }

  ArgumentsConsumer arguments(annotation.getArguments());

  if (arguments.empty()) {
    reportWrongNumberOfArgumentsError(decl, annotation.getKind());
    return true;
  }

  llvm::SmallVector<std::string, 1> identifiers;
  while (auto value = arguments.take<LiteralValue::Kind::String>()) {
    llvm::StringRef identifier = value->getString();
    if (!clang::isValidIdentifier(identifier)) {
      Diagnostics::report(decl,
                          Diagnostics::Kind::AnnotationInvalidSpellingError)
          << toString(annotation.getKind()) << identifier;
      continue;
    }
    identifiers.push_back(identifier.str());
  }
  if (auto value = arguments.take()) {
    reportWrongArgumentTypeError(decl, annotation.getKind(), *value);
  }

  switch (annotation.getKind().value()) {
  case AnnotationKind::GetterFor:
    getter_for.insert(identifiers.begin(), identifiers.end());
    break;
  case AnnotationKind::SetterFor:
    setter_for.insert(identifiers.begin(), identifiers.end());
    break;
  default:
    llvm_unreachable("Unexpected annotation kind.");
  }

  return true;
}

bool AnnotatedConstructorDecl::processAnnotation(const clang::Decl *decl,
                                                 const Annotation &annotation) {
  if (AnnotatedFunctionDecl::processAnnotation(decl, annotation))
    return true;

  const auto *constructor = llvm::cast<clang::CXXConstructorDecl>(decl);
  ArgumentsConsumer arguments(annotation.getArguments());
  switch (annotation.getKind().value()) {
  case AnnotationKind::ImplicitConversion: {
    if (constructor->getNumParams() != 1 ||
        !constructor->isConvertingConstructor(/*AllowExplicit=*/true))
      Diagnostics::report(decl,
                          Diagnostics::Kind::AnnotationInvalidForDeclKindError)
          << "non-converting constructor" << toString(annotation);
    if (arguments.empty()) {
      implicit_conversion = true;
    } else if (auto value = arguments.take<LiteralValue::Kind::Boolean>()) {
      implicit_conversion = value->getBoolean();
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(decl, annotation.getKind(), *value);
    }
    break;
  }
  default:
    return false;
  }

  if (arguments)
    reportWrongNumberOfArgumentsError(decl, annotation.getKind());

  return true;
}

bool AnnotatedFieldOrVarDecl::processAnnotation(const clang::Decl *decl,
                                                const Annotation &annotation) {
  if (AnnotatedNamedDecl::processAnnotation(decl, annotation))
    return true;

  ArgumentsConsumer arguments(annotation.getArguments());
  switch (annotation.getKind().value()) {
  case AnnotationKind::Readonly:
    // TODO: `readonly` is only supported for fields and static member
    // variables.
    if (arguments.empty()) {
      readonly = true;
    } else if (auto value = arguments.take<LiteralValue::Kind::Boolean>()) {
      readonly = value->getBoolean();
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(decl, annotation.getKind(), *value);
    }
    break;
  case AnnotationKind::Postamble:
    postamble = true;
    // as `postamble` implies `manual`:
    // fallthrough
  case AnnotationKind::Manual: {
    // Check whether the initializer contains the expected lambda expression.
    manual_bindings = [&]() -> const clang::LambdaExpr * {
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
    if (manual_bindings == nullptr)
      return false;
    if (annotation.getKind() == AnnotationKind::Postamble &&
        !llvm::isa<clang::TranslationUnitDecl>(decl->getDeclContext())) {
      Diagnostics::report(decl, Diagnostics::Kind::OnlyGlobalScopeAllowedError)
          << toString(annotation.getKind());
      return true;
    }
    break;
  }
  default:
    return false;
  }

  if (arguments)
    reportWrongNumberOfArgumentsError(decl, annotation.getKind());
  return true;
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
