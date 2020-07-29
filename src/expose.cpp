#include "genpybind/expose.h"

#include "genpybind/annotated_decl.h"
#include "genpybind/decl_context_graph.h"
#include "genpybind/decl_context_graph_processing.h"
#include "genpybind/string_utils.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
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

std::string genpybind::getFullyQualifiedName(const clang::TypeDecl *decl) {
  const clang::ASTContext &context = decl->getASTContext();
  const clang::QualType qual_type = context.getTypeDeclType(decl);
  auto policy = context.getPrintingPolicy();
  policy.SuppressScope = false;
  policy.AnonymousTagLocations = false;
  policy.PolishForDeclaration = true;
  return clang::TypeName::getFullyQualifiedName(qual_type, context, policy,
                                                /*WithGlobalNsPrefix=*/true);
}

void genpybind::emitSpelling(llvm::raw_ostream &os,
                             const AnnotatedNamedDecl *annotated_decl) {
  assert(annotated_decl != nullptr); // FIXME
  os << '"';
  os.write_escaped(annotated_decl->getSpelling());
  os << '"';
}

TranslationUnitExposer::TranslationUnitExposer(const DeclContextGraph &graph,
                                               AnnotationStorage &annotations)
    : graph(graph), annotations(annotations) {}

void TranslationUnitExposer::emitModule(llvm::raw_ostream &os,
                                        llvm::StringRef name) {
  std::string expose_declarations;
  std::string context_introducers;
  std::string expose_calls;
  llvm::raw_string_ostream expose_declarations_stream(expose_declarations);
  llvm::raw_string_ostream context_introducers_stream(context_introducers);
  llvm::raw_string_ostream expose_calls_stream(expose_calls);

  DiscriminateIdentifiers used_identifiers;
  llvm::DenseMap<const clang::Decl *, std::string> context_identifiers;

  const EnclosingNamedDeclMap parents =
      findEnclosingScopeIntroducingAncestors(graph, annotations);
  // `nullptr` is used in the parent map to indicate the absence of a named
  // ancestor.  Consequently treat `nullptr` as the module root.
  context_identifiers[nullptr] = "root";

  const auto contexts = declContextsSortedByDependencies(graph, parents);
  for (const clang::DeclContext *decl_context : contexts) {
    const clang::Decl *decl = llvm::cast<clang::Decl>(decl_context);
    // Ensure annotation is available for exposer.
    const AnnotatedDecl *annotated_decl = annotations.getOrInsert(decl);
    std::unique_ptr<DeclContextExposer> exposer =
        DeclContextExposer::create(graph, annotations, decl_context);

    auto parent = parents.find(decl_context);
    assert(parent != parents.end() &&
           "context should have an associated ancestor");
    const std::string identifier = used_identifiers.discriminate([&] {
      llvm::SmallString<128> name =
          annotated_decl == nullptr ? llvm::StringRef("context")
                                    : annotated_decl->getFriendlyDeclKindName();
      if (auto const *type_decl = llvm::dyn_cast<clang::TypeDecl>(decl)) {
        name += getFullyQualifiedName(type_decl);
      } else if (auto const *ns_decl = llvm::dyn_cast<clang::NamedDecl>(decl)) {
        name.push_back('_');
        name += ns_decl->getQualifiedNameAsString();
      }
      makeValidIdentifier(name);
      return name;
    }());
    context_identifiers[decl] = identifier;
    auto parent_identifier = context_identifiers.find(parent->getSecond());
    assert(parent_identifier != context_identifiers.end() &&
           "identifier should have been stored at this point");

    expose_declarations_stream << "void expose_" << identifier;
    exposer->emitDeclaration(expose_declarations_stream);
    expose_declarations_stream << '\n';

    // TODO: Move elsewhere...
    expose_declarations_stream << "void expose_" << identifier;
    exposer->emitDefinition(expose_declarations_stream);
    expose_declarations_stream << '\n';

    context_introducers_stream << "auto " << identifier << " = ";
    exposer->emitIntroducer(context_introducers_stream,
                            parent_identifier->getSecond());
    context_introducers_stream << ";\n";

    expose_calls_stream << "expose_" << identifier << "(" << identifier
                        << ");\n";
  }

  os << expose_declarations_stream.str() << "\n"
     << "PYBIND11_MODULE(" << name << ", root) {\n"
     << context_introducers_stream.str() << "\n"
     << expose_calls_stream.str() << "}\n";
}

std::unique_ptr<DeclContextExposer>
DeclContextExposer::create(const DeclContextGraph &graph,
                           const AnnotationStorage &annotations,
                           const clang::DeclContext *decl_context) {
  assert(decl_context != nullptr);
  const auto *decl = llvm::cast<clang::Decl>(decl_context);
  if (const auto *ns = llvm::dyn_cast<clang::NamespaceDecl>(decl))
    return std::make_unique<NamespaceExposer>(annotations, ns);
  if (const auto *en = llvm::dyn_cast<clang::EnumDecl>(decl))
    return std::make_unique<EnumExposer>(annotations, en);
  if (const auto *rec = llvm::dyn_cast<clang::RecordDecl>(decl))
    return std::make_unique<RecordExposer>(graph, annotations, rec);
  if (DeclContextGraph::accepts(decl))
    return std::make_unique<UnnamedContextExposer>(decl_context);

  llvm_unreachable("Unknown declaration context kind.");
}

UnnamedContextExposer::UnnamedContextExposer(
    const clang::DeclContext * /*decl*/) {}

void UnnamedContextExposer::emitDeclaration(llvm::raw_ostream &os) {
  os << "(::pybind11::module& /*context*/);";
}

void UnnamedContextExposer::emitIntroducer(llvm::raw_ostream &os,
                                           llvm::StringRef parent_identifier) {
  os << parent_identifier;
}

void UnnamedContextExposer::emitDefinition(llvm::raw_ostream &os) {
  os << "(::pybind11::module& /*context*/) {}";
}

NamespaceExposer::NamespaceExposer(const AnnotationStorage &annotations,
                                   const clang::NamespaceDecl *decl)
    : annotated_decl(llvm::dyn_cast_or_null<AnnotatedNamespaceDecl>(
          annotations.get(decl))) {}

void NamespaceExposer::emitDeclaration(llvm::raw_ostream &os) {
  os << "(::pybind11::module& /*context*/);";
}

void NamespaceExposer::emitIntroducer(llvm::raw_ostream &os,
                                      llvm::StringRef parent_identifier) {
  os << parent_identifier;
  // TODO: Emit only for the _first_ declaration of this namespace.
  if (annotated_decl != nullptr && annotated_decl->module) {
    os << ".def_submodule(";
    emitSpelling(os, annotated_decl);
    os << ")";
  }
}

void NamespaceExposer::emitDefinition(llvm::raw_ostream &os) {
  os << "(::pybind11::module& /*context*/) {}";
}

EnumExposer::EnumExposer(const AnnotationStorage &annotations,
                         const clang::EnumDecl *decl)
    : annotated_decl(
          llvm::dyn_cast_or_null<AnnotatedEnumDecl>(annotations.get(decl))) {
  assert(annotated_decl != nullptr);
}

void EnumExposer::emitDeclaration(llvm::raw_ostream &os) {
  os << "(";
  emitType(os);
  os << "& /*context*/);";
}

void EnumExposer::emitIntroducer(llvm::raw_ostream &os,
                                 llvm::StringRef parent_identifier) {
  emitType(os);
  os << "(" << parent_identifier << ", ";
  emitSpelling(os, annotated_decl);
  if (annotated_decl->arithmetic)
    os << ", ::pybind11::arithmetic()";
  os << ")";
}

void EnumExposer::emitDefinition(llvm::raw_ostream &os) {
  os << "(";
  emitType(os);
  os << "& /*context*/) {}";
}

void EnumExposer::emitType(llvm::raw_ostream &os) {
  os << "::pybind11::enum_<"
     << getFullyQualifiedName(
            llvm::cast<clang::TypeDecl>(annotated_decl->getDecl()))
     << ">";
}

RecordExposer::RecordExposer(const DeclContextGraph &graph,
                             const AnnotationStorage &annotations,
                             const clang::RecordDecl *decl)
    : graph(graph), annotated_decl(llvm::dyn_cast_or_null<AnnotatedRecordDecl>(
                        annotations.get(decl))) {
  assert(annotated_decl != nullptr);
}

void RecordExposer::emitDeclaration(llvm::raw_ostream &os) {
  os << "(";
  emitType(os);
  os << "& /*context*/);";
}

void RecordExposer::emitIntroducer(llvm::raw_ostream &os,
                                   llvm::StringRef parent_identifier) {
  emitType(os);
  os << "(" << parent_identifier << ", ";
  emitSpelling(os, annotated_decl);
  if (annotated_decl->dynamic_attr)
    os << ", ::pybind11::dynamic_attr()";
  os << ")";
}

void RecordExposer::emitDefinition(llvm::raw_ostream &os) {
  os << "(";
  emitType(os);
  // As a placeholder always add a constructor for now.
  os << "& context) { context.def(::pybind11::init<>(), \"\"); }";
}

void RecordExposer::emitType(llvm::raw_ostream &os) {
  os << "::pybind11::class_<"
     << getFullyQualifiedName(
            llvm::cast<clang::TypeDecl>(annotated_decl->getDecl()));

  // Add all exposed (i.e. part of graph) public bases as arguments.
  if (const auto *decl =
          llvm::dyn_cast<clang::CXXRecordDecl>(annotated_decl->getDecl())) {
    for (const clang::CXXBaseSpecifier &base : decl->bases()) {
      if (base.getAccessSpecifier() != clang::AS_public)
        continue;
      const clang::TagDecl *base_decl =
          base.getType()->getAsTagDecl()->getDefinition();
      if (graph.getNode(base_decl) != nullptr)
        os << ", " << getFullyQualifiedName(base_decl);
    }
  }

  os << ">";
}
