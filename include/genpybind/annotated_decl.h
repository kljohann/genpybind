#pragma once

#include "genpybind/annotations/annotation.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace clang {
class CXXConstructorDecl;
class CXXMethodDecl;
class LambdaExpr;
} // namespace clang

namespace genpybind {

bool hasAnnotations(const clang::Decl *decl, bool allow_empty = true);
llvm::SmallVector<llvm::StringRef, 1>
getAnnotationStrings(const clang::Decl *decl);

/// Return spelling derived from the name of the identifier that names
/// the declaration.
std::string getSpelling(const clang::NamedDecl *decl);

class AnnotatedDecl {
public:
  enum class Kind {
    Named,

    Namespace,
    Enum,
    Class,
    TypeAlias,
    Variable,

    Function,
    ConversionFunction,
    Method,
    Operator,
    Constructor,
    lastFunction = Constructor,
    lastNamed = Constructor,
  };

private:
  Kind kind;

public:
  AnnotatedDecl(Kind kind) : kind(kind) {}
  virtual ~AnnotatedDecl() = default;

  static std::unique_ptr<AnnotatedDecl> create(const clang::NamedDecl *decl);

  /// Process the given annotation and emit potential errors
  /// via the declarations associated diagnostics engine.
  /// \return whether the annotation has been processed (even with errors)
  virtual bool processAnnotation(const clang::Decl *decl,
                                 const annotations::Annotation &annotation) = 0;

  Kind getKind() const { return kind; };

  /// Retrieve annotations from the annotation attributes of the declaration.
  /// Any errors (e.g. invalid annotations) are reported via the declaration's
  /// associated diagnostics engine.
  void processAnnotations(const clang::Decl *decl);
};

class AnnotatedNamedDecl : public AnnotatedDecl {
protected:
  AnnotatedNamedDecl(Kind kind) : AnnotatedDecl(kind) {}

public:
  /// Alternative identifier to use when exposing the declaration.
  std::string spelling;
  /// Explicitly specified visibility of the declaration.
  /// If this is `None` the effective visibility will be determined
  /// based on the visibility of the parent declaration.
  llvm::Optional<bool> visible;

  AnnotatedNamedDecl() : AnnotatedDecl(Kind::Named) {}

  bool processAnnotation(const clang::Decl *decl,
                         const annotations::Annotation &annotation) override;

  static bool classof(const AnnotatedDecl *decl) {
    return classofKind(decl->getKind());
  }
  static constexpr bool classofKind(Kind kind) {
    return kind >= Kind::Named && kind <= Kind::lastNamed;
  }
};

class AnnotatedNamespaceDecl : public AnnotatedNamedDecl {
public:
  bool module = false;
  std::vector<std::string> only_expose_in;

  AnnotatedNamespaceDecl() : AnnotatedNamedDecl(Kind::Namespace) {}

  bool processAnnotation(const clang::Decl *decl,
                         const annotations::Annotation &annotation) override;

  static bool classof(const AnnotatedDecl *decl) {
    return classofKind(decl->getKind());
  }
  static constexpr bool classofKind(Kind kind) {
    return kind == Kind::Namespace;
  }
};

class AnnotatedEnumDecl : public AnnotatedNamedDecl {
public:
  bool arithmetic = false;
  llvm::Optional<bool> export_values;

  AnnotatedEnumDecl() : AnnotatedNamedDecl(Kind::Enum) {}

  bool processAnnotation(const clang::Decl *decl,
                         const annotations::Annotation &annotation) override;

  static bool classof(const AnnotatedDecl *decl) {
    return classofKind(decl->getKind());
  }
  static constexpr bool classofKind(Kind kind) { return kind == Kind::Enum; }
};

class AnnotatedRecordDecl : public AnnotatedNamedDecl {
public:
  bool dynamic_attr = false;
  llvm::SmallPtrSet<const clang::TagDecl *, 1> hide_base;
  llvm::SmallPtrSet<const clang::TagDecl *, 1> inline_base;
  std::string holder_type;

  AnnotatedRecordDecl() : AnnotatedNamedDecl(Kind::Class) {}

  bool processAnnotation(const clang::Decl *decl,
                         const annotations::Annotation &annotation) override;

  static bool classof(const AnnotatedDecl *decl) {
    return classofKind(decl->getKind());
  }
  static constexpr bool classofKind(Kind kind) { return kind == Kind::Class; }
};

class AnnotatedTypedefNameDecl : public AnnotatedNamedDecl {
public:
  /// If the underlying type has default visibility, make it visible explicitly.
  bool encourage = false;
  /// Expose the underlying type at the location of the type alias instead.
  bool expose_here = false;
  std::vector<annotations::Annotation> annotations_to_propagate;

  AnnotatedTypedefNameDecl() : AnnotatedNamedDecl(Kind::TypeAlias) {}

  bool processAnnotation(const clang::Decl *decl,
                         const annotations::Annotation &annotation) override;

  /// Applies all annotations of this declaration to another declaration.
  /// This is used to propagate spelling and visibility in the case of
  /// "expose_here" type aliases.
  void propagateAnnotations(const clang::Decl *decl,
                            AnnotatedDecl &other) const;

  static bool classof(const AnnotatedDecl *decl) {
    return classofKind(decl->getKind());
  }
  static constexpr bool classofKind(Kind kind) {
    return kind == Kind::TypeAlias;
  }
};

class AnnotatedFunctionDecl : public AnnotatedNamedDecl {
protected:
  AnnotatedFunctionDecl(Kind kind) : AnnotatedNamedDecl(kind) {}

public:
  llvm::SmallVector<std::pair<unsigned, unsigned>, 1> keep_alive;
  llvm::SmallSet<unsigned, 1> noconvert;
  llvm::SmallSet<unsigned, 1> required;
  std::string return_value_policy;

  AnnotatedFunctionDecl() : AnnotatedNamedDecl(Kind::Function) {}

  bool processAnnotation(const clang::Decl *decl,
                         const annotations::Annotation &annotation) override;

  static bool classof(const AnnotatedDecl *decl) {
    return classofKind(decl->getKind());
  }
  static constexpr bool classofKind(Kind kind) {
    return kind >= Kind::Function && kind <= Kind::lastFunction;
  }
};

class AnnotatedConversionFunctionDecl final : public AnnotatedFunctionDecl {
public:
  AnnotatedConversionFunctionDecl()
      : AnnotatedFunctionDecl(Kind::ConversionFunction) {}

  static bool classof(const AnnotatedDecl *decl) {
    return classofKind(decl->getKind());
  }
  static constexpr bool classofKind(Kind kind) {
    return kind == Kind::ConversionFunction;
  }
};

class AnnotatedMethodDecl final : public AnnotatedFunctionDecl {
public:
  llvm::SmallSet<std::string, 1> getter_for;
  llvm::SmallSet<std::string, 1> setter_for;

  AnnotatedMethodDecl() : AnnotatedFunctionDecl(Kind::Method) {}

  bool processAnnotation(const clang::Decl *decl,
                         const annotations::Annotation &annotation) override;

  static bool classof(const AnnotatedDecl *decl) {
    return classofKind(decl->getKind());
  }
  static constexpr bool classofKind(Kind kind) {
    return kind == Kind::Method;
  }
};

class AnnotatedOperatorDecl final : public AnnotatedFunctionDecl {
public:
  AnnotatedOperatorDecl() : AnnotatedFunctionDecl(Kind::Operator) {}

  static bool classof(const AnnotatedDecl *decl) {
    return classofKind(decl->getKind());
  }
  static constexpr bool classofKind(Kind kind) {
    return kind == Kind::Operator;
  }
};

class AnnotatedConstructorDecl : public AnnotatedFunctionDecl {
public:
  bool implicit_conversion = false;

  AnnotatedConstructorDecl() : AnnotatedFunctionDecl(Kind::Constructor) {}

  bool processAnnotation(const clang::Decl *decl,
                         const annotations::Annotation &annotation) override;

  static bool classof(const AnnotatedDecl *decl) {
    return classofKind(decl->getKind());
  }
  static constexpr bool classofKind(Kind kind) {
    return kind == Kind::Constructor;
  }
};

class AnnotatedFieldOrVarDecl : public AnnotatedNamedDecl {
public:
  bool readonly = false;
  const clang::LambdaExpr *manual_bindings = nullptr;
  bool postamble = false;

  AnnotatedFieldOrVarDecl() : AnnotatedNamedDecl(Kind::Variable) {}

  bool processAnnotation(const clang::Decl *decl,
                         const annotations::Annotation &annotation) override;

  static bool classof(const AnnotatedDecl *decl) {
    return classofKind(decl->getKind());
  }
  static constexpr bool classofKind(Kind kind) {
    return kind == Kind::Variable;
  }
};

/// Holds and owns annotations s.t. they do not have to be processed multiple
/// times.  Currently only named declarations are checked for annotations.
class AnnotationStorage {
  llvm::DenseMap<const clang::NamedDecl *, std::unique_ptr<AnnotatedDecl>>
      annotations;

public:
  /// Add an entry for the specified `declaration`.
  /// If `declaration` is `nullptr` or unnamed, `nullptr` is returned.
  AnnotatedDecl *getOrInsert(const clang::Decl *declaration);

  /// Return the entry for the specified `declaration`.
  /// If `declaration` is `nullptr`, unnamed or there is no corresponding entry,
  /// `nullptr` is returned.
  const AnnotatedDecl *get(const clang::Decl *declaration) const;
};

} // namespace genpybind
