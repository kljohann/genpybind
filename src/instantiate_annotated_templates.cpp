// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include "genpybind/instantiate_annotated_templates.h"

#include "genpybind/annotated_decl.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Sema/Sema.h>
#include <llvm/Support/Casting.h>

#include <cassert>

using namespace genpybind;

void InstantiateAnnotatedTemplatesASTConsumer::HandleTranslationUnit(
    clang::ASTContext &context) {
  TraverseDecl(context.getTranslationUnitDecl());

  while (!pending.empty()) {
    clang::ClassTemplateSpecializationDecl *specialization = pending.front();
    assert(specialization != nullptr);
    pending.pop();

    done.insert(specialization);
    instantiateSpecialization(specialization);

    TraverseDecl(specialization->getDefinition());
  }
}

void InstantiateAnnotatedTemplatesASTConsumer::instantiateSpecialization(
    clang::ClassTemplateSpecializationDecl *specialization) {
  assert(specialization != nullptr);
  if (auto *definition =
          llvm::cast_or_null<clang::ClassTemplateSpecializationDecl>(
              specialization->getDefinition())) {
    // TODO: Should this one be returned?
    assert(definition == specialization);
    specialization = definition;
  }

  const clang::TemplateSpecializationKind tsk =
      clang::TSK_ExplicitInstantiationDefinition;
  const clang::TemplateSpecializationKind prev_tsk =
      specialization->getTemplateSpecializationKind();

  // For explicit template instantiation declarations it is sufficient to
  // instantiate all members (cf. `Sema::ActOnExplicitInstantiation`).

  // TODO: What should be done about explicit specializations?
  if (prev_tsk != clang::TSK_Undeclared &&
      prev_tsk != clang::TSK_ImplicitInstantiation &&
      prev_tsk != clang::TSK_ExplicitInstantiationDeclaration)
    return;

  // Put instantiation in enclosing namespace of its template (re-use
  // declaration node, as it should not yet be part of a declaration context).
  // It will still be registered as a specialization of its class template.
  // TODO: Can the node always be re-used in these cases?
  if (prev_tsk == clang::TSK_Undeclared ||
      prev_tsk == clang::TSK_ImplicitInstantiation) {
    clang::ClassTemplateDecl *class_template =
        specialization->getSpecializedTemplate();
    // TODO: Consider qualifying the name (`setQualifierInfo`) and using
    // `getEnclosingNamespaceContext` instead, if the `NestedNameSpecifierLoc`
    // is available in `ASTContext`.
    clang::DeclContext *decl_context = class_template->getDeclContext();
    // TODO: setLexicalDeclContext?
    specialization->setDeclContext(decl_context);
    decl_context->addDecl(specialization);
  }

  clang::SourceLocation loc = specialization->getLocation();

  if (prev_tsk == clang::TSK_Undeclared)
    sema->runWithSufficientStackSpace(loc, [&] {
      sema->InstantiateClassTemplateSpecialization(loc, specialization, tsk,
                                                   /*Complain=*/true);
    });

  auto *definition = llvm::cast_or_null<clang::ClassTemplateSpecializationDecl>(
      specialization->getDefinition());
  assert(definition != nullptr && definition == specialization);

  definition->setSpecializationKind(tsk);
  sema->runWithSufficientStackSpace(loc, [&] {
    sema->InstantiateClassTemplateSpecializationMembers(loc, definition, tsk);
  });
}

bool InstantiateAnnotatedTemplatesASTConsumer::VisitTypedefNameDecl(
    const clang::TypedefNameDecl *decl) {
  if (hasAnnotations(decl)) {
    clang::QualType qual_type = decl->getUnderlyingType();
    if (auto *specialization =
            llvm::dyn_cast_or_null<clang::ClassTemplateSpecializationDecl>(
                qual_type->getAsTagDecl()))
      if (done.count(specialization) == 0)
        pending.push(specialization);
  }
  return true;
}

bool InstantiateAnnotatedTemplatesASTConsumer::
    VisitClassTemplateSpecializationDecl(
        clang::ClassTemplateSpecializationDecl *decl) {
  if (done.count(decl) == 0 && hasAnnotations(decl))
    pending.push(decl);
  return true;
}
