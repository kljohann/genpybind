#pragma once

#include "genpybind/annotations/annotation.h"

#include <clang/AST/Decl.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace clang {
class CXXMethodDecl;
class CXXConstructorDecl;
}  // namespace clang

namespace genpybind {

bool hasAnnotations(const clang::Decl *decl);
llvm::SmallVector<llvm::StringRef, 1>
getAnnotationStrings(const clang::Decl *decl);

class AnnotatedDecl {
  const clang::Decl *decl;

public:
  AnnotatedDecl(const clang::Decl *decl) : decl(decl) {}
  virtual ~AnnotatedDecl() = default;

  static std::unique_ptr<AnnotatedDecl> create(const clang::NamedDecl *decl);

  virtual llvm::StringRef getFriendlyDeclKindName() const = 0;
  /// Process the given annotation and emit potential errors
  /// via the declarations associated diagnostics engine.
  /// \return whether the annotation has been processed (even with errors)
  virtual bool processAnnotation(const annotations::Annotation &annotation) = 0;

  const clang::Decl *getDecl() const { return decl; }
  clang::Decl::Kind getKind() const { return decl->getKind(); }

  /// Retrieve annotations from the annotation attributes of the declaration.
  /// Any errors (e.g. invalid annotations) are reported via the declaration's
  /// associated diagnostics engine.
  void processAnnotations();
};

class AnnotatedNamedDecl : public AnnotatedDecl {
public:
  /// Alternative identifier to use when exposing the declaration.
  std::string spelling;
  /// Explicitly specified visibility of the declaration.
  /// If this is `None` the effective visibility will be determined
  /// based on the visibility of the parent declaration.
  llvm::Optional<bool> visible;

  AnnotatedNamedDecl(const clang::NamedDecl *decl);
  llvm::StringRef getFriendlyDeclKindName() const override;
  bool processAnnotation(const annotations::Annotation &annotation) override;

  /// Return user-provided spelling, falling back to the name of the identifier
  /// that names the declaration.
  std::string getSpelling() const;

  static bool classof(const AnnotatedDecl *decl) {
    return clang::NamedDecl::classofKind(decl->getKind());
  }
};

class AnnotatedNamespaceDecl : public AnnotatedNamedDecl {
public:
  bool module = false;

  llvm::StringRef getFriendlyDeclKindName() const override;
  bool processAnnotation(const annotations::Annotation &annotation) override;

  using AnnotatedNamedDecl::AnnotatedNamedDecl;
  static bool classof(const AnnotatedDecl *decl) {
    return clang::NamespaceDecl::classofKind(decl->getKind());
  }
};

class AnnotatedEnumDecl : public AnnotatedNamedDecl {
public:
  bool arithmetic = false;
  llvm::Optional<bool> export_values;

  llvm::StringRef getFriendlyDeclKindName() const override;
  bool processAnnotation(const annotations::Annotation &annotation) override;

  using AnnotatedNamedDecl::AnnotatedNamedDecl;
  static bool classof(const AnnotatedDecl *decl) {
    return clang::EnumDecl::classofKind(decl->getKind());
  }
};

class AnnotatedRecordDecl : public AnnotatedNamedDecl {
public:
  bool dynamic_attr = false;
  llvm::SmallPtrSet<const clang::TagDecl *, 1> hide_base;
  llvm::SmallPtrSet<const clang::TagDecl *, 1> inline_base;

  llvm::StringRef getFriendlyDeclKindName() const override;
  bool processAnnotation(const annotations::Annotation &annotation) override;

  using AnnotatedNamedDecl::AnnotatedNamedDecl;
  static bool classof(const AnnotatedDecl *decl) {
    return clang::RecordDecl::classofKind(decl->getKind());
  }
};

class AnnotatedTypedefNameDecl : public AnnotatedNamedDecl {
public:
  /// If the underlying type has default visibility, make it visible explicitly.
  bool encourage = false;
  /// Expose the underlying type at the location of the type alias instead.
  bool expose_here = false;
  std::vector<annotations::Annotation> annotations_to_propagate;

  AnnotatedTypedefNameDecl(const clang::TypedefNameDecl *decl);
  llvm::StringRef getFriendlyDeclKindName() const override;
  bool processAnnotation(const annotations::Annotation &annotation) override;

  static bool classof(const AnnotatedDecl *decl) {
    return clang::TypedefNameDecl::classofKind(decl->getKind());
  }

  /// Applies all annotations of this declaration to another declaration.
  /// This is used to propagate spelling and visibility in the case of
  /// "expose_here" type aliases.
  void propagateAnnotations(AnnotatedDecl &other) const;
};

class AnnotatedFunctionDecl : public AnnotatedNamedDecl {
public:
  llvm::SmallVector<std::pair<unsigned, unsigned>, 1> keep_alive;
  llvm::SmallSet<unsigned, 1> noconvert;
  llvm::SmallSet<unsigned, 1> required;
  llvm::Optional<std::string> return_value_policy;

  AnnotatedFunctionDecl(const clang::FunctionDecl *decl);
  llvm::StringRef getFriendlyDeclKindName() const override;
  bool processAnnotation(const annotations::Annotation &annotation) override;

  static bool classof(const AnnotatedDecl *decl);
};

class AnnotatedMethodDecl final : public AnnotatedFunctionDecl {
public:
  llvm::SmallSet<std::string, 1> getter_for;
  llvm::SmallSet<std::string, 1> setter_for;

  AnnotatedMethodDecl(const clang::CXXMethodDecl *decl);
  llvm::StringRef getFriendlyDeclKindName() const override;
  bool processAnnotation(const annotations::Annotation &annotation) override;

  static bool classof(const AnnotatedDecl *decl);
};

class AnnotatedConstructorDecl : public AnnotatedFunctionDecl {
public:
  bool implicit_conversion = false;

  AnnotatedConstructorDecl(const clang::CXXConstructorDecl *decl);
  llvm::StringRef getFriendlyDeclKindName() const override;
  bool processAnnotation(const annotations::Annotation &annotation) override;

  static bool classof(const AnnotatedDecl *decl);
};

class AnnotatedFieldOrVarDecl : public AnnotatedNamedDecl {
public:
  bool readonly = false;

  AnnotatedFieldOrVarDecl(const clang::FieldDecl *decl);
  AnnotatedFieldOrVarDecl(const clang::VarDecl *decl);

  llvm::StringRef getFriendlyDeclKindName() const override;
  bool processAnnotation(const annotations::Annotation &annotation) override;

  static bool classof(const AnnotatedDecl *decl) {
    return clang::FieldDecl::classofKind(decl->getKind()) ||
           clang::VarDecl::classofKind(decl->getKind());
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
