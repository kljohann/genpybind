#include "genpybind/expose.h"

#include "genpybind/annotated_decl.h"
#include "genpybind/decl_context_graph.h"
#include "genpybind/decl_context_graph_processing.h"
#include "genpybind/diagnostics.h"
#include "genpybind/string_utils.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/QualTypeNames.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Specifiers.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>

using namespace genpybind;

namespace {

class DiscriminateIdentifiers {
  llvm::StringMap<unsigned> discriminators;

public:
  std::string discriminate(llvm::StringRef name) {
    auto it = discriminators.try_emplace(name, 0).first;
    const unsigned discriminator = ++it->getValue();
    std::string result{name};
    if (discriminator != 1)
      result += "_" + llvm::utostr(discriminator);
    return result;
  }
};

} // namespace

static void emitStringLiteral(llvm::raw_ostream &os, llvm::StringRef text) {
  os << '"';
  os.write_escaped(text);
  os << '"';
}

static llvm::StringRef getBriefText(const clang::Decl *decl) {
  const clang::ASTContext &context = decl->getASTContext();
  if (const clang::RawComment *raw = context.getRawCommentForAnyRedecl(decl)) {
    return raw->getBriefText(context);
  }
  return {};
}

static llvm::StringRef getDocstring(const clang::FunctionDecl *function) {
  llvm::StringRef result = getBriefText(function);
  if (result.empty())
    if (const clang::FunctionTemplateDecl *primary =
            function->getPrimaryTemplate())
      result = getBriefText(primary);
  return result;
}

static clang::PrintingPolicy
getPrintingPolicyForExposedNames(const clang::ASTContext &context) {
  auto policy = context.getPrintingPolicy();
  policy.SuppressScope = false;
  policy.AnonymousTagLocations = false;
  policy.PolishForDeclaration = true;
  return policy;
}

static void emitParameterTypes(llvm::raw_ostream &os,
                               const clang::FunctionDecl *function) {
  const clang::ASTContext &context = function->getASTContext();
  auto policy = getPrintingPolicyForExposedNames(context);
  bool comma = false;
  for (const clang::ParmVarDecl *param : function->parameters()) {
    if (comma)
      os << ", ";
    os << clang::TypeName::getFullyQualifiedName(param->getOriginalType(),
                                                 context, policy,
                                                 /*WithGlobalNsPrefix=*/true);
    comma = true;
  }
}

static void emitParameters(llvm::raw_ostream &os,
                           const AnnotatedFunctionDecl *annotation) {
  const auto *function = llvm::cast<clang::FunctionDecl>(annotation->getDecl());
  unsigned index = 0;
  for (const clang::ParmVarDecl *param : function->parameters()) {
    // TODO: Do not emit `arg()` for `pybind11::{kw,}args`.
    os << ", ::pybind11::arg(";
    emitStringLiteral(os, param->getName());
    os << ")";
    if (annotation->noconvert.count(index) != 0)
      os << ".noconvert()";
    if (annotation->required.count(index) != 0)
      os << ".none(false)";
    // TODO: Emit default value.
    ++index;
  }
}

static void emitFunctionPointer(llvm::raw_ostream &os,
                                const clang::FunctionDecl *function) {
  // TODO: All names need to be printed in a fully-qualified way (also nested
  // template arguments)
  auto policy = getPrintingPolicyForExposedNames(function->getASTContext());
  os << "::pybind11::overload_cast<";
  emitParameterTypes(os, function);
  os << ">(&::";
  function->printQualifiedName(os, policy);
  if (const clang::TemplateArgumentList *args =
          function->getTemplateSpecializationArgs()) {
    clang::printTemplateArgumentList(os, args->asArray(), policy);
  }
  if (function->getType()->castAs<clang::FunctionType>()->isConst())
    os << ", ::pybind11::const_";
  os << ")";
}

std::string genpybind::getFullyQualifiedName(const clang::TypeDecl *decl) {
  const clang::ASTContext &context = decl->getASTContext();
  const clang::QualType qual_type = context.getTypeDeclType(decl);
  auto policy = getPrintingPolicyForExposedNames(context);
  return clang::TypeName::getFullyQualifiedName(qual_type, context, policy,
                                                /*WithGlobalNsPrefix=*/true);
}

TranslationUnitExposer::TranslationUnitExposer(
    clang::Sema &sema, const DeclContextGraph &graph,
    const EffectiveVisibilityMap &visibilities, AnnotationStorage &annotations)
    : sema(sema), graph(graph), visibilities(visibilities),
      annotations(annotations) {}

void TranslationUnitExposer::emitModule(llvm::raw_ostream &os,
                                        llvm::StringRef name) {
  const EnclosingNamedDeclMap parents =
      findEnclosingScopeIntroducingAncestors(graph, annotations);

  const clang::DeclContext *cycle = nullptr;
  const auto sorted_contexts =
      declContextsSortedByDependencies(graph, parents, &cycle);
  if (cycle != nullptr) {
    // TODO: Report this before any other output, ideally pointing to the
    // typedef name decl for `expose_here` cycles.
    Diagnostics::report(llvm::cast<clang::Decl>(cycle),
                        Diagnostics::Kind::ExposeHereCycleError);
    return;
  }

  llvm::DenseMap<const clang::DeclContext *, std::string> context_identifiers(
      static_cast<unsigned>(sorted_contexts.size() + 1));
  // `nullptr` is used in the parent map to indicate the absence of a named
  // ancestor.  Consequently treat `nullptr` as the module root.
  context_identifiers[nullptr] = "root";

  struct WorklistItem {
    const clang::DeclContext *decl_context;
    std::unique_ptr<DeclContextExposer> exposer;
    llvm::StringRef identifier;
  };

  std::vector<WorklistItem> worklist;
  worklist.reserve(sorted_contexts.size());

  {
    llvm::DenseMap<const clang::NamespaceDecl *, const clang::DeclContext *>
        covered_namespaces;
    DiscriminateIdentifiers used_identifiers;
    for (const clang::DeclContext *decl_context : sorted_contexts) {
      // Since the decls to expose in each context are discovered via the name
      // lookup mechanism below, it's sufficient to visit every loookup context
      // once, instead of e.g. visiting transparent/non-lookup contexts or each
      // re-opened namespace.
      if (!decl_context->isLookupContext())
        continue;
      if (const auto *ns = llvm::dyn_cast<clang::NamespaceDecl>(decl_context)) {
        auto inserted = covered_namespaces.try_emplace(
            ns->getOriginalNamespace(), decl_context);
        if (!inserted.second) {
          auto existing_identifier =
              context_identifiers.find(inserted.first->getSecond());
          assert(existing_identifier != context_identifiers.end() &&
                 "identifier should have been stored at this point");
          auto result = context_identifiers.try_emplace(
              decl_context, existing_identifier->getSecond());
          assert(result.second);
          continue;
        }
      }

      llvm::SmallString<128> name("context");
      if (auto const *type_decl =
              llvm::dyn_cast<clang::TypeDecl>(decl_context)) {
        name += getFullyQualifiedName(type_decl);
      } else if (auto const *ns_decl =
                     llvm::dyn_cast<clang::NamedDecl>(decl_context)) {
        name.push_back('_');
        name += ns_decl->getQualifiedNameAsString();
      }
      makeValidIdentifier(name);
      auto result = context_identifiers.try_emplace(
          decl_context, used_identifiers.discriminate(name));
      assert(result.second);
      llvm::StringRef identifier = result.first->getSecond();
      worklist.push_back(
          {decl_context,
           DeclContextExposer::create(graph, annotations, decl_context),
           identifier});
    }
  }

  // Emit declarations for `expose_` functions
  for (const auto &item : worklist) {
    os << "void expose_" << item.identifier << "(";
    item.exposer->emitParameter(os);
    os << ");\n";
  }

  os << '\n';

  // Emit module definition
  os << "PYBIND11_MODULE(" << name << ", root) {\n";

  // Emit context introducers
  for (const auto &item : worklist) {
    auto parent = parents.find(item.decl_context);
    assert(parent != parents.end() &&
           "context should have an associated ancestor");
    auto parent_identifier = context_identifiers.find(
        llvm::cast_or_null<clang::DeclContext>(parent->getSecond()));
    assert(parent_identifier != context_identifiers.end() &&
           "identifier should have been stored at this point");

    os << "auto " << item.identifier << " = ";
    item.exposer->emitIntroducer(os, parent_identifier->getSecond());
    os << ";\n";
  }

  os << '\n';

  // Emit calls to `expose_` functions
  for (const auto &item : worklist) {
    os << "expose_" << item.identifier << "(" << item.identifier << ");\n";
  }

  os << "}\n\n";

  // Emit definitions for `expose_` functions
  // (can be partitioned into several files in the future)
  for (const auto &item : worklist) {
    os << "void expose_" << item.identifier << "(";
    item.exposer->emitParameter(os);
    os << ") {\n";

    auto handle_decl = [&](const clang::NamedDecl *proposed_decl) {
      // Use default visibility of original (possibly transparent) decl context.
      bool default_visibility = [&] {
        auto it = visibilities.find(proposed_decl->getDeclContext());
        return it != visibilities.end() ? it->getSecond() : false;
      }();
      const auto *annotation = llvm::dyn_cast<AnnotatedNamedDecl>(
          annotations.getOrInsert(proposed_decl));
      item.exposer->handleDecl(os, proposed_decl, annotation,
                               default_visibility);
    };

    // TODO: Sort / make deterministic
    std::vector<const clang::NamedDecl *> decls =
        collectVisibleDeclsFromDeclContext(sema, item.decl_context,
                                           item.exposer->inliningPolicy());

    for (const clang::NamedDecl *proposed_decl : decls) {
      // If there are several declarations of a function template,
      // only one is picked up here.  Thus all specializations can be
      // processed unconditionally.
      if (const auto *tpl =
              llvm::dyn_cast<clang::FunctionTemplateDecl>(proposed_decl)) {
        llvm::for_each(tpl->specializations(), handle_decl);
      } else {
        handle_decl(proposed_decl);
      }
    }

    item.exposer->finalizeDefinition(os);
    os << "}\n\n";
  }
}

std::unique_ptr<DeclContextExposer>
DeclContextExposer::create(const DeclContextGraph &graph,
                           AnnotationStorage &annotations,
                           const clang::DeclContext *decl_context) {
  assert(decl_context != nullptr);
  const auto *decl = llvm::cast<clang::Decl>(decl_context);
  if (const AnnotatedDecl *annotated_decl = annotations.getOrInsert(decl)) {
    if (const auto *ad = llvm::dyn_cast<AnnotatedNamespaceDecl>(annotated_decl))
      return std::make_unique<NamespaceExposer>(ad);
    if (const auto *ad = llvm::dyn_cast<AnnotatedEnumDecl>(annotated_decl))
      return std::make_unique<EnumExposer>(ad);
    if (const auto *ad = llvm::dyn_cast<AnnotatedRecordDecl>(annotated_decl))
      return std::make_unique<RecordExposer>(graph, ad);
  }
  if (DeclContextGraph::accepts(decl))
    return std::make_unique<DeclContextExposer>();

  llvm_unreachable("Unknown declaration context kind.");
}

llvm::Optional<RecordInliningPolicy>
DeclContextExposer::inliningPolicy() const {
  return llvm::None;
}

void DeclContextExposer::emitParameter(llvm::raw_ostream &os) {
  os << "::pybind11::module& context";
}

void DeclContextExposer::emitIntroducer(llvm::raw_ostream &os,
                                        llvm::StringRef parent_identifier) {
  os << parent_identifier;
}

void DeclContextExposer::handleDecl(llvm::raw_ostream &os,
                                    const clang::NamedDecl *decl,
                                    const AnnotatedNamedDecl *annotation,
                                    bool default_visibility) {
  assert(decl != nullptr);
  assert(annotation != nullptr);
  if (!annotation->visible.getValueOr(default_visibility))
    return;

  return handleDeclImpl(os, decl, annotation);
}

void DeclContextExposer::handleDeclImpl(llvm::raw_ostream &os,
                                        const clang::NamedDecl *decl,
                                        const AnnotatedNamedDecl *annotation) {
  assert(!llvm::isa<AnnotatedConstructorDecl>(annotation) &&
         "constructors are handled in RecordExposer");
  const clang::ASTContext &ast_context = decl->getASTContext();
  auto printing_policy = getPrintingPolicyForExposedNames(ast_context);

  if (const auto *annot =
          llvm::dyn_cast<AnnotatedTypedefNameDecl>(annotation)) {
    // Type aliases are hidden by default and do not inherit the default
    // visibility, thus a second check is necessary here.
    if (!annot->visible.hasValue())
      return;
    if (annot->expose_here)
      return;

    const auto *alias = llvm::cast<clang::TypedefNameDecl>(decl);
    const clang::TagDecl *target = alias->getUnderlyingType()->getAsTagDecl();
    if (target == nullptr)
      return;

    os << "context.attr(";
    emitStringLiteral(os, annot->getSpelling());
    os << ") = ::genpybind::getObjectForType<" << getFullyQualifiedName(target)
       << ">();\n";

    return;
  }

  if (const auto *annot = llvm::dyn_cast<AnnotatedFieldOrVarDecl>(annotation)) {
    // For fields and static member variables see `RecordExposer`.
    assert(!llvm::isa<clang::FieldDecl>(decl) &&
           "should have been processed by RecordExposer");
    os << "context.attr(";
    emitStringLiteral(os, annot->getSpelling());
    os << ") = ::";
    decl->printQualifiedName(os, printing_policy);
    os << ";\n";
    return;
  }

  if (const auto *annot = llvm::dyn_cast<AnnotatedFunctionDecl>(annotation)) {
    const auto *function = llvm::cast<clang::FunctionDecl>(decl);
    const auto *method = llvm::dyn_cast<clang::CXXMethodDecl>(decl);

    // Operators are handled separately in `RecordExposer::finalizeDefinition`.
    if (function->isOverloadedOperator())
      return;

    os << ((method != nullptr && method->isStatic()) ? "context.def_static("
                                                     : "context.def(");
    emitStringLiteral(os, annot->getSpelling());
    os << ", ";
    emitFunctionPointer(os, function);
    os << ", ";
    emitStringLiteral(os, getDocstring(function));
    emitParameters(os, annot);
    // TODO: Emit policies
    os << ");\n";
  }
}

void DeclContextExposer::finalizeDefinition(llvm::raw_ostream &os) {
  os << "(void)context;\n";
}

NamespaceExposer::NamespaceExposer(const AnnotatedNamespaceDecl *annotated_decl)
    : annotated_decl(annotated_decl) {}

void NamespaceExposer::emitIntroducer(llvm::raw_ostream &os,
                                      llvm::StringRef parent_identifier) {
  os << parent_identifier;
  if (annotated_decl != nullptr && annotated_decl->module) {
    os << ".def_submodule(";
    emitStringLiteral(os, annotated_decl->getSpelling());
    os << ")";
  }
}

EnumExposer::EnumExposer(const AnnotatedEnumDecl *annotated_decl)
    : annotated_decl(annotated_decl) {
  assert(annotated_decl != nullptr);
}

void EnumExposer::emitParameter(llvm::raw_ostream &os) {
  emitType(os);
  os << "& context";
}

void EnumExposer::emitIntroducer(llvm::raw_ostream &os,
                                 llvm::StringRef parent_identifier) {
  emitType(os);
  os << "(" << parent_identifier << ", ";
  emitStringLiteral(os, annotated_decl->getSpelling());
  if (annotated_decl->arithmetic)
    os << ", ::pybind11::arithmetic()";
  os << ")";
}

void EnumExposer::finalizeDefinition(llvm::raw_ostream &os) {
  const auto *decl = llvm::cast<clang::EnumDecl>(annotated_decl->getDecl());
  if (annotated_decl->export_values.getValueOr(!decl->isScoped()))
    os << "context.export_values();\n";
}

void EnumExposer::emitType(llvm::raw_ostream &os) {
  os << "::pybind11::enum_<"
     << getFullyQualifiedName(
            llvm::cast<clang::TypeDecl>(annotated_decl->getDecl()))
     << ">";
}

void EnumExposer::handleDeclImpl(llvm::raw_ostream &os,
                                 const clang::NamedDecl *decl,
                                 const AnnotatedNamedDecl *annotation) {
  if (const auto *enumerator = llvm::dyn_cast<clang::EnumConstantDecl>(decl)) {
    const auto *enum_decl =
        llvm::cast<clang::EnumDecl>(annotated_decl->getDecl());
    const std::string scope = getFullyQualifiedName(enum_decl);
    os << "context.value(";
    emitStringLiteral(os, annotation->getSpelling());
    os << ", " << scope << "::" << enumerator->getName() << ");\n";
  }
}

RecordExposer::RecordExposer(const DeclContextGraph &graph,
                             const AnnotatedRecordDecl *annotated_decl)
    : graph(graph), annotated_decl(annotated_decl) {
  assert(annotated_decl != nullptr);
}

llvm::Optional<RecordInliningPolicy> RecordExposer::inliningPolicy() const {
  return RecordInliningPolicy::createFromAnnotation(*annotated_decl);
}

void RecordExposer::emitParameter(llvm::raw_ostream &os) {
  emitType(os);
  os << "& context";
}

void RecordExposer::emitIntroducer(llvm::raw_ostream &os,
                                   llvm::StringRef parent_identifier) {
  emitType(os);
  os << "(" << parent_identifier << ", ";
  emitStringLiteral(os, annotated_decl->getSpelling());
  if (annotated_decl->dynamic_attr)
    os << ", ::pybind11::dynamic_attr()";
  os << ")";
}

void RecordExposer::finalizeDefinition(llvm::raw_ostream &os) {
  os << "context.doc() = ";
  emitStringLiteral(os, getBriefText(annotated_decl->getDecl()));
  os << ";\n";
}

void RecordExposer::emitType(llvm::raw_ostream &os) {
  os << "::pybind11::class_<"
     << getFullyQualifiedName(
            llvm::cast<clang::TypeDecl>(annotated_decl->getDecl()));

  // Add all exposed (i.e. part of graph) public bases as arguments.
  if (const auto *decl =
          llvm::dyn_cast<clang::CXXRecordDecl>(annotated_decl->getDecl())) {
    for (const clang::CXXBaseSpecifier &base : decl->bases()) {
      const clang::TagDecl *base_decl =
          base.getType()->getAsTagDecl()->getDefinition();
      if (base.getAccessSpecifier() != clang::AS_public ||
          annotated_decl->hide_base.count(base_decl) != 0 ||
          annotated_decl->inline_base.count(base_decl) != 0 ||
          graph.getNode(base_decl) == nullptr)
        continue;
      os << ", " << getFullyQualifiedName(base_decl);
    }
  }

  os << ">";
}

void RecordExposer::handleDeclImpl(llvm::raw_ostream &os,
                                   const clang::NamedDecl *decl,
                                   const AnnotatedNamedDecl *annotation) {
  const auto *record_decl =
      llvm::cast<clang::CXXRecordDecl>(annotated_decl->getDecl());
  const clang::ASTContext &ast_context = decl->getASTContext();
  const auto printing_policy = getPrintingPolicyForExposedNames(ast_context);

  if (const auto *annot =
          llvm::dyn_cast<AnnotatedConstructorDecl>(annotation)) {
    const auto *constructor = llvm::dyn_cast<clang::CXXConstructorDecl>(decl);
    if (constructor->isMoveConstructor() || record_decl->isAbstract())
      return;

    if (annot->implicit_conversion) {
      clang::QualType from_qual_type = constructor->getParamDecl(0)->getType();
      clang::QualType to_qual_type = ast_context.getTypeDeclType(record_decl);
      os << "::pybind11::implicitly_convertible<"
         << clang::TypeName::getFullyQualifiedName(from_qual_type, ast_context,
                                                   printing_policy,
                                                   /*WithGlobalNsPrefix=*/true)
         << ", "
         << clang::TypeName::getFullyQualifiedName(to_qual_type, ast_context,
                                                   printing_policy,
                                                   /*WithGlobalNsPrefix=*/true)
         << ">();\n";
    }

    os << "context.def(::pybind11::init<";
    emitParameterTypes(os, constructor);
    os << ">(), ";
    emitStringLiteral(os, getDocstring(constructor));
    emitParameters(os, annot);
    // TODO: Emit policies
    os << ");\n";
    return;
  }

  if (const auto *annot = llvm::dyn_cast<AnnotatedFieldOrVarDecl>(annotation)) {
    clang::QualType type = llvm::cast<clang::ValueDecl>(decl)->getType();
    bool readonly = type.isConstQualified() || annot->readonly;
    os << (readonly ? "context.def_readonly" : "context.def_readwrite");
    os << (llvm::isa<clang::VarDecl>(decl) ? "_static(" : "(");
    emitStringLiteral(os, annotation->getSpelling());
    os << ", &::";
    decl->printQualifiedName(os, printing_policy);
    os << ");\n";
    return;
  }

  // Fall back to generic implementation.
  DeclContextExposer::handleDeclImpl(os, decl, annotation);
}
