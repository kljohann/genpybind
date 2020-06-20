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

llvm::StringRef AnnotatedTypedefNameDecl::getFriendlyDeclKindName() const {
  return "type alias";
}

bool AnnotatedTypedefNameDecl::processAnnotation(const Annotation &annotation) {
  if (AnnotatedNamedDecl::processAnnotation(annotation))
    return true;

  ArgumentsConsumer arguments(annotation.getArguments());
  switch (annotation.getKind().value()) {
  case AnnotationKind::ExposeHere:
    expose_here = true;
    break;
  default:
    return false;
  }
  if (arguments)
    reportWrongNumberOfArgumentsError(getDecl(), annotation.getKind());
  return true;
}
