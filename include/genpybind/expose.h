// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "genpybind/decl_context_graph_processing.h"
#include "genpybind/visible_decls.h"

#include <llvm/ADT/StringRef.h>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace clang {
class ASTContext;
class CXXMethodDecl;
class CXXRecordDecl;
class DeclContext;
class EnumDecl;
class FunctionDecl;
class NamedDecl;
class NamespaceDecl;
class QualType;
class Sema;
class TypeDecl;
enum OverloadedOperatorKind : int;
} // namespace clang
namespace llvm {
class raw_ostream;
template <typename T> class SmallVectorImpl;
} // namespace llvm

namespace genpybind {
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

  void emitModule(std::vector<llvm::raw_ostream *> ostreams,
                  llvm::StringRef module_name);
};

class DeclContextExposer {
protected:
  const AnnotationStorage &annotations;

public:
  DeclContextExposer(const AnnotationStorage &annotations);
  virtual ~DeclContextExposer() = default;

  static std::unique_ptr<DeclContextExposer>
  create(const DeclContextGraph &graph, const AnnotationStorage &annotations,
         const clang::DeclContext *decl_context);

  virtual std::optional<RecordInliningPolicy> inliningPolicy() const;
  virtual void emitParameter(llvm::raw_ostream &os);
  virtual void emitIntroducer(llvm::raw_ostream &os,
                              llvm::StringRef parent_identifier);
  void handleDecl(llvm::raw_ostream &os, const clang::NamedDecl *decl,
                  bool default_visibility);
  virtual void finalizeDefinition(llvm::raw_ostream &os);

protected:
  virtual void handleDeclImpl(llvm::raw_ostream &os,
                              const clang::NamedDecl *decl);
};

class NamespaceExposer : public DeclContextExposer {
  const clang::NamespaceDecl *namespace_decl;

public:
  NamespaceExposer(const clang::NamespaceDecl *namespace_decl,
                   const AnnotationStorage &annotations);

  void emitIntroducer(llvm::raw_ostream &os,
                      llvm::StringRef parent_identifier) override;
};

class EnumExposer : public DeclContextExposer {
  const clang::EnumDecl *enum_decl;

public:
  EnumExposer(const clang::EnumDecl *enum_decl,
              const AnnotationStorage &annotations);

  void emitParameter(llvm::raw_ostream &os) override;
  void emitIntroducer(llvm::raw_ostream &os,
                      llvm::StringRef parent_identifier) override;
  void finalizeDefinition(llvm::raw_ostream &os) override;

private:
  void emitType(llvm::raw_ostream &os);
  void handleDeclImpl(llvm::raw_ostream &os,
                      const clang::NamedDecl *decl) override;
};

class RecordExposer : public DeclContextExposer {
  const clang::CXXRecordDecl *record_decl;
  const DeclContextGraph &graph;
  RecordInliningPolicy inlining_policy;
  struct Property {
    const clang::CXXMethodDecl *getter = nullptr;
    const clang::CXXMethodDecl *setter = nullptr;
  };
  std::map<std::string, Property> properties;

public:
  RecordExposer(const clang::CXXRecordDecl *record_decl,
                const DeclContextGraph &graph,
                const AnnotationStorage &annotations,
                RecordInliningPolicy inlining_policy);

  std::optional<RecordInliningPolicy> inliningPolicy() const override;
  void emitParameter(llvm::raw_ostream &os) override;
  void emitIntroducer(llvm::raw_ostream &os,
                      llvm::StringRef parent_identifier) override;
  void finalizeDefinition(llvm::raw_ostream &os) override;

private:
  void emitProperties(llvm::raw_ostream &os);
  void emitOperator(llvm::raw_ostream &os, const clang::FunctionDecl *function);
  static void emitOperatorDefinition(
      llvm::raw_ostream &os, const clang::ASTContext &ast_context,
      clang::OverloadedOperatorKind kind,
      const llvm::SmallVectorImpl<clang::QualType> &parameter_types,
      clang::QualType return_type, bool reverse_parameters);
  void emitType(llvm::raw_ostream &os);
  void handleDeclImpl(llvm::raw_ostream &os,
                      const clang::NamedDecl *decl) override;
};

} // namespace genpybind
