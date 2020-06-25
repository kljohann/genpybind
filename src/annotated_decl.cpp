#include "genpybind/annotated_decl.h"

#include "genpybind/annotations/parser.h"
#include "genpybind/diagnostics.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Attr.h>
#include <clang/Basic/CharInfo.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/Error.h>

using namespace genpybind;

using annotations::Annotation;
using annotations::AnnotationKind;
using annotations::LiteralValue;
using annotations::Parser;

static const char *kLozenge = "◊";

bool genpybind::hasAnnotations(const clang::Decl *decl) {
  return llvm::any_of(decl->specific_attrs<clang::AnnotateAttr>(),
                      [](const auto *attr) {
                        return attr->getAnnotation().startswith(kLozenge);
                      });
}

static Parser::Annotations parseAnnotations(const ::clang::Decl *decl) {
  Parser::Annotations annotations;

  for (const auto *attr : decl->specific_attrs<::clang::AnnotateAttr>()) {
    ::llvm::StringRef annotation_text = attr->getAnnotation();
    if (!annotation_text.consume_front(kLozenge))
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
      : remaining(std::move(remaining)) {}

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
AnnotatedDecl::create(const clang::NamedDecl *decl) {
  assert(decl != nullptr);
  if (const auto *td = llvm::dyn_cast<clang::TypedefNameDecl>(decl))
    return std::make_unique<AnnotatedTypedefNameDecl>(td);
  return std::make_unique<AnnotatedNamedDecl>(
      llvm::cast<clang::NamedDecl>(decl));
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
  if (!llvm::isa<clang::NamespaceDecl>(decl) && hasAnnotations(decl))
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
        spelling = text;
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

llvm::StringRef AnnotatedNamedDecl::getSpelling() const {
  return spelling.empty() ? llvm::cast<clang::NamedDecl>(getDecl())->getName()
                          : llvm::StringRef(spelling);
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

AnnotatedDecl *AnnotationStorage::getOrInsert(const clang::NamedDecl *decl) {
  assert(decl != nullptr);
  auto result = annotations.try_emplace(decl, AnnotatedDecl::create(decl));
  AnnotatedDecl *annotated_decl = result.first->getSecond().get();
  assert(annotated_decl != nullptr);
  if (result.second)
    annotated_decl->processAnnotations();
  return annotated_decl;
}

/// Return the entry for the specified declaration.
/// If there is no entry, `nullptr` is returned.
const AnnotatedDecl *
AnnotationStorage::get(const clang::NamedDecl *decl) const {
  assert(decl != nullptr);
  auto it = annotations.find(decl);
  if (it != annotations.end())
    return it->second.get();
  return nullptr;
}
