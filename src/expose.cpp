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
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
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
#include <utility>

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

/// A very unprincipled attempt to print a default argument expression in
/// a fully qualified way by augmenting and replicating the behavior/output of
/// clang::Stmt::prettyPrint.
/// Where successful, the global namespace specifier (`::`) is prepended to all
/// nested name specifiers.
/// Many useful facilities are not exposed in the clang API, e.g. the helper
/// functions in `QualTypeNames.cpp` and parts of `TreeTransform.h`.  If they
/// are at some point, this hack should be replaced altogether.
struct AttemptFullQualificationPrinter : clang::PrinterHelper {
  const clang::ASTContext &context;
  clang::PrintingPolicy printing_policy;
  bool within_braced_initializer = false;

  AttemptFullQualificationPrinter(const clang::ASTContext &context,
                                  clang::PrintingPolicy printing_policy)
      : context(context), printing_policy(printing_policy) {}

  bool handledStmt(clang::Stmt *stmt, llvm::raw_ostream &os) override {
    const auto *expr = llvm::dyn_cast<clang::Expr>(stmt);
    if (expr == nullptr)
      return false;

    auto printRecursively = [&](const clang::Expr *expr) {
      expr->printPretty(os, this, printing_policy, /*Indentation=*/0,
                        /*NewlineSymbol=*/"\n", &context);
    };

    // Prepend the outermost braced initializer with its fully qualified type.
    bool should_prepend_qualified_type_to_braced_initializer = [&] {
      if (within_braced_initializer)
        return false;
      if (llvm::isa<clang::InitListExpr>(expr))
        return true;
      if (llvm::isa<clang::CXXTemporaryObjectExpr>(expr))
        return false;
      if (const auto *constr = llvm::dyn_cast<clang::CXXConstructExpr>(expr))
        return constr->isListInitialization();
      return false;
    }();

    if (should_prepend_qualified_type_to_braced_initializer) {
      os << clang::TypeName::getFullyQualifiedName(expr->getType(), context,
                                                   printing_policy,
                                                   /*WithGlobalNsPrefix=*/true);
      within_braced_initializer = true;
      printRecursively(expr);
      within_braced_initializer = false;
      return true;
    }

    // Fully qualify all printed types.  Effectively all instances of
    // `getType().print(...)` in `StmtPrinter` have to be re-defined.
    // TODO: It seems the existing implementation mostly does the right thing
    // already, but does not insert global namespace specifiers.  As the
    // bindings live at the global namespace this should not be a problem,
    // though.  Investigate and consider removing this code.
    // TODO: Extend to remaining cases, or replace by more scalable solution.
    // - OffsetOfExpr
    // - CompoundLiteralExpr
    // - ConvertVectorExpr
    // - ImplicitValueInitExpr
    // - VAArgExpr
    // - BuiltinBitCastExpr
    // - CXXTypeidExpr
    // - CXXUuidofExpr
    // - CXXScalarValueInitExpr
    // - CXXUnresolvedConstructExpr
    // - TypeTraitExpr
    // - RequiresExpr
    // - ObjCBridgedCastExpr
    // - BlockExpr
    // - AsTypeExpr

    auto printFullyQualifiedName = [&](clang::QualType qual_type) {
      os << clang::TypeName::getFullyQualifiedName(qual_type, context,
                                                   printing_policy,
                                                   /*WithGlobalNsPrefix=*/true);
    };

    if (const auto *cast = llvm::dyn_cast<clang::CStyleCastExpr>(expr)) {
      os << '(';
      printFullyQualifiedName(cast->getType());
      os << ')';
      printRecursively(cast->getSubExpr());
      return true;
    }

    if (const auto *cast = llvm::dyn_cast<clang::CXXNamedCastExpr>(expr)) {
      os << cast->getCastName() << '<';
      printFullyQualifiedName(cast->getType());
      os << ">(";
      printRecursively(cast->getSubExpr());
      os << ')';
      return true;
    }

    if (const auto *cast = llvm::dyn_cast<clang::CXXFunctionalCastExpr>(expr)) {
      printFullyQualifiedName(cast->getType());
      if (cast->getLParenLoc().isValid())
        os << '(';
      printRecursively(cast->getSubExpr());
      if (cast->getLParenLoc().isValid())
        os << ')';
      return true;
    }

    if (const auto *temporary =
            llvm::dyn_cast<clang::CXXTemporaryObjectExpr>(expr)) {
      printFullyQualifiedName(temporary->getType());

      if (temporary->isStdInitListInitialization())
        /* do nothing; braces are printed by containing expression */;
      else
        os << (temporary->isListInitialization() ? '{' : '(');

      bool comma = false;
      for (const clang::Expr *argument : temporary->arguments()) {
        if (argument->isDefaultArgument())
          break;
        if (comma)
          os << ", ";
        printRecursively(argument);
        comma = true;
      }

      if (temporary->isStdInitListInitialization())
        /* do nothing */;
      else
        os << (temporary->isListInitialization() ? '}' : ')');

      return true;
    }

    // Fully qualify nested name specifiers.  This corresponds to calls to
    // `getQualifier()` in `StmtPrinter`, which have to be re-defined.

    if (const auto *decl_ref = llvm::dyn_cast<clang::DeclRefExpr>(expr)) {
      os << "::";
      decl_ref->getFoundDecl()->printNestedNameSpecifier(os, printing_policy);
      if (decl_ref->hasTemplateKeyword())
        os << "template ";
      os << decl_ref->getNameInfo();
      if (decl_ref->hasExplicitTemplateArgs())
        clang::printTemplateArgumentList(os, decl_ref->template_arguments(),
                                         printing_policy);
      return true;
    }

    if (const auto *member = llvm::dyn_cast<clang::MemberExpr>(expr)) {
      assert(!printing_policy.SuppressImplicitBase &&
             "suppressing implicit 'this' not implemented");
      const clang::Expr *base = member->getBase();
      printRecursively(base);

      bool suppress_member_access_operator = [&] {
        if (const auto *parent_member = llvm::dyn_cast<clang::MemberExpr>(base))
          if (const auto *field = llvm::dyn_cast<clang::FieldDecl>(
                  parent_member->getMemberDecl()))
            return field->isAnonymousStructOrUnion();
        return false;
      }();

      if (!suppress_member_access_operator)
        os << (member->isArrow() ? "->" : ".");

      os << "::";
      member->getFoundDecl()->printNestedNameSpecifier(os, printing_policy);
      if (member->hasTemplateKeyword())
        os << "template ";
      os << member->getMemberNameInfo();
      if (member->hasExplicitTemplateArgs())
        clang::printTemplateArgumentList(os, member->template_arguments(),
                                         printing_policy);
      return true;
    }

    return false;
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
  const clang::ASTContext &context = function->getASTContext();
  auto printing_policy = getPrintingPolicyForExposedNames(context);
  AttemptFullQualificationPrinter printer_helper{context, printing_policy};
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
    if (const clang::Expr *expr = param->getDefaultArg()) {
      // os << "/* = ";
      os << " = ";
      expr->printPretty(os, &printer_helper, printing_policy, /*Indentation=*/0,
                        /*NewlineSymbol=*/"\n", &context);
      // os << " */";
    }
    ++index;
  }
}

static void emitPolicies(llvm::raw_ostream &os,
                         const AnnotatedFunctionDecl *annotation) {
  if (!annotation->return_value_policy.empty())
    os << ", pybind11::return_value_policy::"
       << annotation->return_value_policy;
  for (const auto &item : annotation->keep_alive) {
    os << ", pybind11::keep_alive<" << item.first << ", " << item.second
       << ">()";
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

static void emitManualBindings(llvm::raw_ostream &os,
                               const clang::ASTContext &context,
                               const clang::LambdaExpr *manual_bindings) {
  assert(manual_bindings != nullptr);
  auto printing_policy = getPrintingPolicyForExposedNames(context);
  // Print and immediately invoke manual bindings lambda(IIFE).
  // TODO: This requires all referenced types and declarations in the manual
  // binding code to be fully qualified.  It would be useful to atleast warn if
  // this is not the case.  Unfortunately, automatically qualifying these
  // references is not straight forward.  See `AttemptFullQualificationPrinter`,
  // which could be a starting point except for the fact that it has no
  // influence on the printing of decls within the lambda expression.
  const clang::CXXMethodDecl *method = manual_bindings->getCallOperator();
  assert(method->getNumParams() == 1);
  const clang::ParmVarDecl *param = method->getParamDecl(0);
  bool needs_context = param->isUsed() || param->isReferenced();
  if (needs_context)
    os << "[](auto &" << param->getName() << ") ";
  manual_bindings->getBody()->printPretty(os, nullptr, printing_policy,
                                          /*Indentation=*/0,
                                          /*NewlineSymbol=*/"\n", &context);
  if (needs_context)
    os << "(context);\n";
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

  { // Emit 'postamble' manual bindings.
    const clang::DeclContext *decl_context = graph.getRoot()->getDeclContext();
    for (clang::DeclContext::specific_decl_iterator<clang::VarDecl>
             it(decl_context->decls_begin()),
         end_it(decl_context->decls_end());
         it != end_it; ++it) {
      const auto *annotation =
          llvm::cast<AnnotatedFieldOrVarDecl>(annotations.getOrInsert(*it));
      if (!annotation->postamble || annotation->manual_bindings == nullptr)
        continue;
      const clang::ASTContext &ast_context = it->getASTContext();
      os << "\n";
      emitManualBindings(os, ast_context, annotation->manual_bindings);
    }
  }

  os << "}\n\n";

  // Emit definitions for `expose_` functions
  // (can be partitioned into several files in the future)
  for (const auto &item : worklist) {
    os << "void expose_" << item.identifier << "(";
    item.exposer->emitParameter(os);
    os << ") {\n";

    // TODO: As the declarations possibly come from nested non-lookup contexts,
    // in principle the default visibility of their original decl context would
    // need to be considered.  But this is only relevant if the decl is nested
    // inside an `ExportDecl`, since this is the only non-lookup context that
    // has an effect on default visibility.  On the other hand, for inlined
    // decls, the default visibility of the current lookup context should be
    // considered.  For now, ignore the `ExportDecl` case for simplicity.
    bool default_visibility = [&] {
      auto it = visibilities.find(item.decl_context);
      return it != visibilities.end() ? it->getSecond() : false;
    }();

    auto handle_decl = [&](const clang::NamedDecl *proposed_decl) {
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
    if (annot->manual_bindings != nullptr) {
      if (!annot->postamble)
        emitManualBindings(os, ast_context, annot->manual_bindings);
      return;
    }
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

  if (const auto *annot = llvm::dyn_cast<AnnotatedMethodDecl>(annotation)) {
    // Properties are handled separately in `RecordExposer::finalizeDefinition`.
    if (!annot->getter_for.empty() || !annot->setter_for.empty())
      return;
  }

  if (const auto *annot = llvm::dyn_cast<AnnotatedFunctionDecl>(annotation)) {
    const auto *function = llvm::cast<clang::FunctionDecl>(decl);
    const auto *method = llvm::dyn_cast<clang::CXXMethodDecl>(decl);

    // Operators are handled separately in `RecordExposer::finalizeDefinition`.
    if (function->isDeleted() || function->isOverloadedOperator())
      return;

    os << ((method != nullptr && method->isStatic()) ? "context.def_static("
                                                     : "context.def(");
    emitStringLiteral(os, annot->getSpelling());
    os << ", ";
    emitFunctionPointer(os, function);
    os << ", ";
    emitStringLiteral(os, getDocstring(function));
    emitParameters(os, annot);
    emitPolicies(os, annot);
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
  emitProperties(os);
  os << "context.doc() = ";
  emitStringLiteral(os, getBriefText(annotated_decl->getDecl()));
  os << ";\n";
}

void RecordExposer::emitProperties(llvm::raw_ostream &os) {
  // TODO: Sort / make deterministic
  for (const auto &entry : properties) {
    llvm::StringRef name = entry.getKey();
    const Property &property = entry.getValue();
    if (property.getter == nullptr) {
      if (property.setter == nullptr)
        continue;
      Diagnostics::report(property.setter,
                          Diagnostics::Kind::PropertyHasNoGetterError)
          << name;
      continue;
    }
    bool writable = property.setter != nullptr;
    os << (writable ? "context.def_property("
                    : "context.def_property_readonly(");
    emitStringLiteral(os, name);
    os << ", ";
    emitFunctionPointer(os, property.getter);
    if (writable) {
      os << ", ";
      emitFunctionPointer(os, property.setter);
    }
    os << ");\n";
  }
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

  if (!annotated_decl->holder_type.empty()) {
    os << ", " << annotated_decl->holder_type;
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
    if (constructor->isMoveConstructor() || constructor->isDeleted() ||
        record_decl->isAbstract())
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
    emitPolicies(os, annot);
    os << ");\n";
    return;
  }

  // If the method should be turned into a property, remember this for later.
  if (const auto *annot = llvm::dyn_cast<AnnotatedMethodDecl>(annotation)) {
    const auto *method = llvm::cast<clang::CXXMethodDecl>(decl);
    if (!annot->getter_for.empty() || !annot->setter_for.empty()) {
      for (auto const &name : annot->getter_for) {
        const clang::CXXMethodDecl *previous =
            std::exchange(properties[name].getter, method);
        if (previous != nullptr) {
          Diagnostics::report(decl,
                              Diagnostics::Kind::PropertyAlreadyDefinedError)
              << name << 0U;
          Diagnostics::report(previous, clang::diag::note_previous_definition);
        }
      }
      for (auto const &name : annot->setter_for) {
        const clang::CXXMethodDecl *previous =
            std::exchange(properties[name].setter, method);
        if (previous != nullptr) {
          Diagnostics::report(decl,
                              Diagnostics::Kind::PropertyAlreadyDefinedError)
              << name << 1U;
          Diagnostics::report(previous, clang::diag::note_previous_definition);
        }
      }
      return;
    }
  }

  if (const auto *annot = llvm::dyn_cast<AnnotatedFieldOrVarDecl>(annotation)) {
    if (annot->manual_bindings != nullptr) {
      assert(!annot->postamble && "postamble only allowed in global scope");
      emitManualBindings(os, ast_context, annot->manual_bindings);
      return;
    }
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
