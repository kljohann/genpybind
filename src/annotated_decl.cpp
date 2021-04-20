#include "genpybind/annotated_decl.h"

#include "genpybind/annotations/literal_value.h"
#include "genpybind/annotations/parser.h"
#include "genpybind/diagnostics.h"
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

std::unique_ptr<AnnotatedDecl>
AnnotatedDecl::create(const clang::NamedDecl *named_decl) {
  assert(named_decl != nullptr);
  if (const auto *decl = llvm::dyn_cast<clang::TypedefNameDecl>(named_decl))
    return std::make_unique<AnnotatedTypedefNameDecl>(decl);
  if (const auto *decl = llvm::dyn_cast<clang::NamespaceDecl>(named_decl))
    return std::make_unique<AnnotatedNamespaceDecl>(decl);
  if (const auto *decl = llvm::dyn_cast<clang::EnumDecl>(named_decl))
    return std::make_unique<AnnotatedEnumDecl>(decl);
  if (const auto *decl = llvm::dyn_cast<clang::RecordDecl>(named_decl))
    return std::make_unique<AnnotatedRecordDecl>(decl);
  if (const auto *decl = llvm::dyn_cast<clang::FieldDecl>(named_decl))
    return std::make_unique<AnnotatedFieldOrVarDecl>(decl);
  if (const auto *decl = llvm::dyn_cast<clang::VarDecl>(named_decl))
    return std::make_unique<AnnotatedFieldOrVarDecl>(decl);

  if (llvm::isa<clang::CXXDeductionGuideDecl>(named_decl) ||
      llvm::isa<clang::CXXDestructorDecl>(named_decl)) {
    // Fall through to generic AnnotatedNamedDecl case.
  } else if (const auto *decl =
                 llvm::dyn_cast<clang::CXXConversionDecl>(named_decl)) {
    // Conversion decls are treated as functions, since they do not
    // support the extra property annotations available for methods.
    return std::make_unique<AnnotatedFunctionDecl>(decl);
  } else if (const auto *decl =
                 llvm::dyn_cast<clang::CXXConstructorDecl>(named_decl)) {
    return std::make_unique<AnnotatedConstructorDecl>(decl);
  } else if (const auto *decl =
                 llvm::dyn_cast<clang::CXXMethodDecl>(named_decl)) {
    return std::make_unique<AnnotatedMethodDecl>(decl);
  } else if (const auto *decl =
                 llvm::dyn_cast<clang::FunctionDecl>(named_decl)) {
    return std::make_unique<AnnotatedFunctionDecl>(decl);
  }

  return std::make_unique<AnnotatedNamedDecl>(named_decl);
}

void AnnotatedDecl::processAnnotations() {
  const Parser::Annotations annotations = parseAnnotations(decl);
  for (const Annotation &annotation : annotations) {
    if (!processAnnotation(annotation)) {
      Diagnostics::report(decl,
                          Diagnostics::Kind::AnnotationInvalidForDeclKindError)
          << getFriendlyDeclKindName() << toString(annotation);
    }
  }
}

AnnotatedNamedDecl::AnnotatedNamedDecl(const clang::NamedDecl *decl)
    : AnnotatedDecl(decl) {
  assert(decl != nullptr);
  // Non-namespace named decls that have at least one annotation
  // are visible by default.  This can be overruled by an explicit
  // `visible(default)`, `visible(false)` or `hidden` annotation.
  if (!llvm::isa<clang::NamespaceDecl>(decl) &&
      hasAnnotations(decl, /*allow_empty=*/false))
    visible = true;
}

llvm::StringRef AnnotatedNamedDecl::getFriendlyDeclKindName() const {
  return "named declaration";
}

bool AnnotatedNamedDecl::processAnnotation(const Annotation &annotation) {
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
      reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);
    }
    break;
  case AnnotationKind::ExposeAs:
    if (arguments.empty()) {
      reportWrongNumberOfArgumentsError(getDecl(), annotation.getKind());
    } else if (arguments.take<LiteralValue::Kind::Default>()) {
      spelling.clear();
    } else if (auto value = arguments.take<LiteralValue::Kind::String>()) {
      llvm::StringRef text = value->getString();
      if (clang::isValidIdentifier(text)) {
        spelling = text.str();
      } else {
        Diagnostics::report(getDecl(),
                            Diagnostics::Kind::AnnotationInvalidSpellingError)
            << toString(AnnotationKind::ExposeAs) << text;
      }
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);
    }
    break;
  default:
    return false;
  }
  if (arguments)
    reportWrongNumberOfArgumentsError(getDecl(), annotation.getKind());
  return true;
}

std::string AnnotatedNamedDecl::getSpelling() const {
  if (!spelling.empty())
    return spelling;

  llvm::SmallString<128> result;
  llvm::raw_svector_ostream os(result);

  const auto *const decl = llvm::cast<clang::NamedDecl>(getDecl());
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

llvm::StringRef AnnotatedNamespaceDecl::getFriendlyDeclKindName() const {
  return "namespace";
}

bool AnnotatedNamespaceDecl::processAnnotation(const Annotation &annotation) {
  if (AnnotatedNamedDecl::processAnnotation(annotation))
    return true;

  ArgumentsConsumer arguments(annotation.getArguments());
  switch (annotation.getKind().value()) {
  case AnnotationKind::Module:
    if (arguments.empty()) {
      module = true;
    } else if (auto value = arguments.take<LiteralValue::Kind::String>()) {
      module = true;
      AnnotatedNamedDecl::processAnnotation(
          Annotation(AnnotationKind::ExposeAs, {*value}));
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);
    }
    break;
  case AnnotationKind::OnlyExposeIn:
    if (arguments.empty())
      reportWrongNumberOfArgumentsError(getDecl(), annotation.getKind());
    while (auto value = arguments.take<LiteralValue::Kind::String>())
      only_expose_in.push_back(value->getString());
    if (auto value = arguments.take())
      reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);
    break;
  default:
    return false;
  }
  if (arguments)
    reportWrongNumberOfArgumentsError(getDecl(), annotation.getKind());
  return true;
}

llvm::StringRef AnnotatedEnumDecl::getFriendlyDeclKindName() const {
  return "enumeration";
}

bool AnnotatedEnumDecl::processAnnotation(const Annotation &annotation) {
  if (AnnotatedNamedDecl::processAnnotation(annotation))
    return true;

  ArgumentsConsumer arguments(annotation.getArguments());
  switch (annotation.getKind().value()) {
  case AnnotationKind::Arithmetic:
    if (arguments.empty()) {
      arithmetic = true;
    } else if (auto value = arguments.take<LiteralValue::Kind::Boolean>()) {
      arithmetic = value->getBoolean();
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);
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
      reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);
    }
    break;
  default:
    return false;
  }
  if (arguments)
    reportWrongNumberOfArgumentsError(getDecl(), annotation.getKind());
  return true;
}

llvm::StringRef AnnotatedRecordDecl::getFriendlyDeclKindName() const {
  return "record";
}

bool AnnotatedRecordDecl::processAnnotation(const Annotation &annotation) {
  if (AnnotatedNamedDecl::processAnnotation(annotation))
    return true;

  ArgumentsConsumer arguments(annotation.getArguments());
  switch (annotation.getKind().value()) {
  case AnnotationKind::DynamicAttr:
    if (arguments.empty()) {
      dynamic_attr = true;
    } else if (auto value = arguments.take<LiteralValue::Kind::Boolean>()) {
      dynamic_attr = value->getBoolean();
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);
    }
    break;
  case AnnotationKind::HideBase: {
    std::vector<std::string> names;
    while (auto value = arguments.take<LiteralValue::Kind::String>())
      names.push_back(value->getString());
    if (auto value = arguments.take())
      reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);

    clang::ast_matchers::internal::HasNameMatcher matcher(names);
    bool found_match = false;
    // TODO: More verbose error if this is not the case?
    if (const auto *decl = llvm::dyn_cast<clang::CXXRecordDecl>(getDecl())) {
      for (const clang::CXXBaseSpecifier &base : decl->bases()) {
        const clang::TagDecl *base_decl =
            base.getType()->getAsTagDecl()->getDefinition();
        if (names.empty() || matcher.matchesNode(*base_decl)) {
          hide_base.insert(base_decl);
          found_match = true;
        }
      }
    }
    if (!found_match) {
      Diagnostics::report(
          getDecl(),
          Diagnostics::Kind::AnnotationContainsUnknownBaseTypeWarning)
          << toString(annotation.getKind());
    }
    break;
  }
  case AnnotationKind::HolderType:
    if (auto value = arguments.take<LiteralValue::Kind::String>()) {
      holder_type = value->getString();
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);
    }
    break;
  case AnnotationKind::InlineBase: {
    std::vector<std::string> names;
    while (auto value = arguments.take<LiteralValue::Kind::String>())
      names.push_back(value->getString());
    if (auto value = arguments.take())
      reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);

    clang::ast_matchers::internal::HasNameMatcher matcher(names);
    bool found_match = false;
    // TODO: More verbose error if this is not the case?
    if (const auto *decl = llvm::dyn_cast<clang::CXXRecordDecl>(getDecl())) {
      decl->forallBases([&](const clang::CXXRecordDecl *base_decl) -> bool {
        base_decl = base_decl->getDefinition();
        if (names.empty() || matcher.matchesNode(*base_decl)) {
          inline_base.insert(base_decl);
          found_match = true;
        }
        return true; // continue visiting other bases
      });
    }
    if (!found_match) {
      Diagnostics::report(
          getDecl(),
          Diagnostics::Kind::AnnotationContainsUnknownBaseTypeWarning)
          << toString(annotation.getKind());
    }
    break;
  }
  default:
    return false;
  }
  if (arguments)
    reportWrongNumberOfArgumentsError(getDecl(), annotation.getKind());
  return true;
}

AnnotatedTypedefNameDecl::AnnotatedTypedefNameDecl(
    const clang::TypedefNameDecl *decl)
    : AnnotatedNamedDecl(decl) {
  assert(decl != nullptr);
  const clang::TagDecl *target_decl = decl->getUnderlyingType()->getAsTagDecl();

  if (hasAnnotations(decl) && target_decl == nullptr)
    Diagnostics::report(decl, Diagnostics::Kind::UnsupportedAliasTargetError);
}

llvm::StringRef AnnotatedTypedefNameDecl::getFriendlyDeclKindName() const {
  return "type alias";
}

bool AnnotatedTypedefNameDecl::processAnnotation(const Annotation &annotation) {
  if (AnnotatedNamedDecl::processAnnotation(annotation)) {
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
    reportWrongNumberOfArgumentsError(getDecl(), annotation.getKind());

  if (encourage && expose_here)
    Diagnostics::report(getDecl(),
                        Diagnostics::Kind::ConflictingAnnotationsError)
        << toString(AnnotationKind::Encourage)
        << toString(AnnotationKind::ExposeHere);

  return true;
}

void AnnotatedTypedefNameDecl::propagateAnnotations(
    AnnotatedDecl &other) const {
  // Always propagate the effective spelling of the type alias, which is the
  // name of its identifier if no explicit `expose_as` annotation has been
  // given.  If there is one, it will be processed twice, but this is benign.
  other.processAnnotation(Annotation(
      AnnotationKind::ExposeAs, {LiteralValue::createString(getSpelling())}));
  // As the `visible` annotatation is implicit if there is at least one other
  // annotation, its computed value also has to be propagated, as it's possible
  // that there is no explicit annotation.
  other.processAnnotation(
      Annotation(AnnotationKind::Visible,
                 {visible.hasValue() ? LiteralValue::createBoolean(*visible)
                                     : LiteralValue::createDefault()}));
  for (const Annotation &annotation : annotations_to_propagate) {
    bool result = other.processAnnotation(annotation);
    // So far, only annotations that are valid for all `NamedDecl`s are
    // propagated.  If additional annotations are forwarded in the future,
    // they need to be checked for validity when processing the
    // `TypedefNameDecl`'s annotations (see note above).
    assert(result && "invalid annotation has been propagated");
    static_cast<void>(result);
  }
}

AnnotatedFunctionDecl::AnnotatedFunctionDecl(const clang::FunctionDecl *decl)
    : AnnotatedNamedDecl(decl) {}

llvm::StringRef AnnotatedFunctionDecl::getFriendlyDeclKindName() const {
  const auto *decl = llvm::cast<clang::FunctionDecl>(getDecl());
  if (decl->isOverloadedOperator())
    return "operator";
  if (llvm::isa<clang::CXXConversionDecl>(decl))
    return "conversion function";
  return "free function";
}

bool AnnotatedFunctionDecl::processAnnotation(const Annotation &annotation) {
  if (AnnotatedNamedDecl::processAnnotation(annotation))
    return true;

  ArgumentsConsumer arguments(annotation.getArguments());
  std::vector<llvm::StringRef> parameter_names;
  switch (annotation.getKind().value()) {
  case AnnotationKind::KeepAlive:
    if (arguments.size() != 2) {
      reportWrongNumberOfArgumentsError(getDecl(), annotation.getKind());
      return true;
    }
    parameter_names.emplace_back("return");
    if (llvm::isa<clang::CXXMethodDecl>(getDecl()))
      parameter_names.emplace_back("this");
    break;
  case AnnotationKind::Noconvert:
  case AnnotationKind::Required:
    if (arguments.empty()) {
      reportWrongNumberOfArgumentsError(getDecl(), annotation.getKind());
      return true;
    }
    break;
  case AnnotationKind::ReturnValuePolicy: {
    if (arguments.size() != 1) {
      reportWrongNumberOfArgumentsError(getDecl(), annotation.getKind());
    } else if (auto value = arguments.take<LiteralValue::Kind::String>()) {
      return_value_policy = value->getString();
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);
    }
    return true;
  }
  default:
    return false;
  }

  const auto *decl = llvm::cast<clang::FunctionDecl>(getDecl());
  for (const clang::ParmVarDecl *param : decl->parameters()) {
    parameter_names.push_back(param->getName());
  }

  bool success = true;
  llvm::SmallVector<unsigned, 2> parameter_indices;
  while (auto value = arguments.take<LiteralValue::Kind::String>()) {
    auto it = llvm::find(parameter_names, value->getString());
    if (it == parameter_names.end()) {
      Diagnostics::report(
          getDecl(), Diagnostics::Kind::AnnotationInvalidArgumentSpecifierError)
          << toString(annotation.getKind()) << value->getString();
      success = false;
      continue;
    }
    parameter_indices.push_back(
        static_cast<unsigned>(std::distance(parameter_names.begin(), it)));
  }
  if (auto value = arguments.take()) {
    reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);
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

bool AnnotatedFunctionDecl::classof(const AnnotatedDecl *decl) {
  return clang::FunctionDecl::classofKind(decl->getKind()) &&
         !(clang::CXXDeductionGuideDecl::classofKind(decl->getKind()) ||
           clang::CXXDestructorDecl::classofKind(decl->getKind()));
}

AnnotatedMethodDecl::AnnotatedMethodDecl(const clang::CXXMethodDecl *decl)
    : AnnotatedFunctionDecl(decl) {
  assert(classof(this) && "expected CXXMethodDecl, not derived class");
}

llvm::StringRef AnnotatedMethodDecl::getFriendlyDeclKindName() const {
  const auto *decl = llvm::cast<clang::FunctionDecl>(getDecl());
  if (decl->isOverloadedOperator())
    return "operator";
  return "method";
}

bool AnnotatedMethodDecl::processAnnotation(const Annotation &annotation) {
  if (AnnotatedFunctionDecl::processAnnotation(annotation))
    return true;

  auto report_invalid_signature = [&]() {
    Diagnostics::report(getDecl(),
                        Diagnostics::Kind::AnnotationIncompatibleSignatureError)
        << getFriendlyDeclKindName() << toString(annotation.getKind());
  };

  const auto *decl = llvm::cast<clang::FunctionDecl>(getDecl());

  // Property-annotations are not valid for overloaded operators.
  if (decl->isOverloadedOperator())
    return false;

  clang::QualType return_type = decl->getReturnType();
  switch (annotation.getKind().value()) {
  case AnnotationKind::GetterFor:
    if (decl->getMinRequiredArguments() != 0 || return_type->isVoidType())
      report_invalid_signature();
    break;
  case AnnotationKind::SetterFor:
    if (decl->getNumParams() < 1 || decl->getMinRequiredArguments() > 1)
      report_invalid_signature();
    break;
  default:
    return false;
  }

  ArgumentsConsumer arguments(annotation.getArguments());

  if (arguments.empty()) {
    reportWrongNumberOfArgumentsError(getDecl(), annotation.getKind());
    return true;
  }

  llvm::SmallVector<std::string, 1> identifiers;
  while (auto value = arguments.take<LiteralValue::Kind::String>()) {
    llvm::StringRef identifier = value->getString();
    if (!clang::isValidIdentifier(identifier)) {
      Diagnostics::report(getDecl(),
                          Diagnostics::Kind::AnnotationInvalidSpellingError)
          << toString(annotation.getKind()) << identifier;
      continue;
    }
    identifiers.push_back(identifier.str());
  }
  if (auto value = arguments.take()) {
    reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);
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

bool AnnotatedMethodDecl::classof(const AnnotatedDecl *decl) {
  // The extra annotations only apply to methods, not to derived classes
  // such as constructors.  Therefore `AnnotatedConstructorDecl` directly
  // derives from `AnnotatedFunctionDecl`.
  return decl->getKind() == clang::Decl::Kind::CXXMethod;
}

AnnotatedConstructorDecl::AnnotatedConstructorDecl(
    const clang::CXXConstructorDecl *decl)
    : AnnotatedFunctionDecl(decl) {}

llvm::StringRef AnnotatedConstructorDecl::getFriendlyDeclKindName() const {
  return "constructor";
}

bool AnnotatedConstructorDecl::processAnnotation(const Annotation &annotation) {
  if (AnnotatedFunctionDecl::processAnnotation(annotation))
    return true;

  const auto* constructor = llvm::cast<clang::CXXConstructorDecl>(getDecl());
  ArgumentsConsumer arguments(annotation.getArguments());
  switch (annotation.getKind().value()) {
  case AnnotationKind::ImplicitConversion: {
    if (constructor->getNumParams() != 1 ||
        !constructor->isConvertingConstructor(/*AllowExplicit=*/true))
      Diagnostics::report(getDecl(),
                          Diagnostics::Kind::AnnotationInvalidForDeclKindError)
          << "non-converting constructor" << toString(annotation);
    if (arguments.empty()) {
      implicit_conversion = true;
    } else if (auto value = arguments.take<LiteralValue::Kind::Boolean>()) {
      implicit_conversion = value->getBoolean();
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);
    }
    break;
  }
  default:
    return false;
  }

  if (arguments)
    reportWrongNumberOfArgumentsError(getDecl(), annotation.getKind());

  return true;
}

bool AnnotatedConstructorDecl::classof(const AnnotatedDecl *decl) {
  return clang::CXXConstructorDecl::classofKind(decl->getKind());
}

AnnotatedFieldOrVarDecl::AnnotatedFieldOrVarDecl(const clang::FieldDecl *decl)
    : AnnotatedNamedDecl(decl) {}

AnnotatedFieldOrVarDecl::AnnotatedFieldOrVarDecl(const clang::VarDecl *decl)
    : AnnotatedNamedDecl(decl) {}

llvm::StringRef AnnotatedFieldOrVarDecl::getFriendlyDeclKindName() const {
  return "variable";
}

bool AnnotatedFieldOrVarDecl::processAnnotation(const Annotation &annotation) {
  if (AnnotatedNamedDecl::processAnnotation(annotation))
    return true;

  ArgumentsConsumer arguments(annotation.getArguments());
  switch (annotation.getKind().value()) {
  case AnnotationKind::Readonly:
    // TODO: `readonly` is only supported for fields and static member variables.
    if (arguments.empty()) {
      readonly = true;
    } else if (auto value = arguments.take<LiteralValue::Kind::Boolean>()) {
      readonly = value->getBoolean();
    } else if ((value = arguments.take())) {
      reportWrongArgumentTypeError(getDecl(), annotation.getKind(), *value);
    }
    break;
  case AnnotationKind::Postamble:
    postamble = true;
    // as `postamble` implies `manual`:
    // fallthrough
  case AnnotationKind::Manual: {
    // Check whether the initializer contains the expected lambda expression.
    manual_bindings = [&]() -> const clang::LambdaExpr * {
      const auto *var = llvm::dyn_cast<clang::VarDecl>(getDecl());
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
        !llvm::isa<clang::TranslationUnitDecl>(getDecl()->getDeclContext())) {
      Diagnostics::report(getDecl(),
                          Diagnostics::Kind::OnlyGlobalScopeAllowedError)
          << toString(annotation.getKind());
      return true;
    }
    break;
  }
  default:
    return false;
  }

  if (arguments)
    reportWrongNumberOfArgumentsError(getDecl(), annotation.getKind());
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
      annotated_decl->processAnnotations();
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
