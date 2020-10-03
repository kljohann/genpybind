#pragma once

#include "genpybind/decl_context_graph_processing.h"
#include "genpybind/visible_decls.h"

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/StringRef.h>

#include <memory>
#include <string>

namespace clang {
class DeclContext;
class EnumDecl;
class NamespaceDecl;
class RecordDecl;
class Sema;
class TypeDecl;
} // namespace clang
namespace llvm {
class raw_ostream;
} // namespace llvm

namespace genpybind {
class AnnotatedEnumDecl;
class AnnotatedNamedDecl;
class AnnotatedNamespaceDecl;
class AnnotatedRecordDecl;
class AnnotationStorage;
class DeclContextGraph;

std::string getFullyQualifiedName(const clang::TypeDecl *decl);

class TranslationUnitExposer {
  clang::Sema &sema;
  const DeclContextGraph &graph;
  const EffectiveVisibilityMap &visibilities;
  AnnotationStorage &annotations;

public:
  TranslationUnitExposer(clang::Sema &sema, const DeclContextGraph &graph,
                         const EffectiveVisibilityMap &visibilities,
                         AnnotationStorage &annotations);

  void emitModule(llvm::raw_ostream &os, llvm::StringRef name);
};

class DeclContextExposer {
public:
  virtual ~DeclContextExposer() = default;

  static std::unique_ptr<DeclContextExposer>
  create(const DeclContextGraph &graph, AnnotationStorage &annotations,
         const clang::DeclContext *decl_context);

  virtual llvm::Optional<RecordInliningPolicy> inliningPolicy() const;
  virtual void emitParameter(llvm::raw_ostream &os);
  virtual void emitIntroducer(llvm::raw_ostream &os,
                              llvm::StringRef parent_identifier);
  void handleDecl(llvm::raw_ostream &os, const clang::NamedDecl *decl,
                  const AnnotatedNamedDecl *annotation,
                  bool default_visibility);
  virtual void finalizeDefinition(llvm::raw_ostream &os);

protected:
  virtual void handleDeclImpl(llvm::raw_ostream &os,
                              const clang::NamedDecl *decl,
                              const AnnotatedNamedDecl *annotation);
};

class NamespaceExposer : public DeclContextExposer {
  const AnnotatedNamespaceDecl *annotated_decl;

public:
  NamespaceExposer(const AnnotatedNamespaceDecl *annotated_decl);

  void emitIntroducer(llvm::raw_ostream &os,
                      llvm::StringRef parent_identifier) override;
};

class EnumExposer : public DeclContextExposer {
  const AnnotatedEnumDecl *annotated_decl;

public:
  EnumExposer(const AnnotatedEnumDecl *annotated_decl);

  void emitParameter(llvm::raw_ostream &os) override;
  void emitIntroducer(llvm::raw_ostream &os,
                      llvm::StringRef parent_identifier) override;
  void finalizeDefinition(llvm::raw_ostream &os) override;

private:
  void emitType(llvm::raw_ostream &os);
  void handleDeclImpl(llvm::raw_ostream &os, const clang::NamedDecl *decl,
                      const AnnotatedNamedDecl *annotation) override;
};

class RecordExposer : public DeclContextExposer {
  const DeclContextGraph &graph;
  const AnnotatedRecordDecl *annotated_decl;

public:
  RecordExposer(const DeclContextGraph &graph,
                const AnnotatedRecordDecl *annotated_decl);

  llvm::Optional<RecordInliningPolicy> inliningPolicy() const override;
  void emitParameter(llvm::raw_ostream &os) override;
  void emitIntroducer(llvm::raw_ostream &os,
                      llvm::StringRef parent_identifier) override;
  void finalizeDefinition(llvm::raw_ostream &os) override;

private:
  void emitType(llvm::raw_ostream &os);
  void handleDeclImpl(llvm::raw_ostream &os, const clang::NamedDecl *decl,
                      const AnnotatedNamedDecl *annotation) override;
};

} // namespace genpybind
