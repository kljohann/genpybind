#pragma once

#include "genpybind/annotations/annotation.h"

#include <clang/AST/Decl.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/Optional.h>

#include <string>
#include <vector>

namespace genpybind {

bool hasAnnotations(const clang::Decl *decl);

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
  /// Any errors (e.g. invalid annotations) are reported via the declarations
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

  static bool classof(const AnnotatedDecl *decl) {
    return clang::NamedDecl::classofKind(decl->getKind());
  }
};

class AnnotatedTypedefNameDecl : public AnnotatedNamedDecl {
public:
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

class AnnotationStorage {
  llvm::DenseMap<const clang::NamedDecl *, std::unique_ptr<AnnotatedDecl>>
      annotations;

public:
  /// Add an entry for the specified declaration.
  AnnotatedDecl *getOrInsert(const clang::NamedDecl *decl);

  /// Return the entry for the specified declaration.
  /// If there is no entry, `nullptr` is returned.
  const AnnotatedDecl *get(const clang::NamedDecl *decl) const;
};

} // namespace genpybind
