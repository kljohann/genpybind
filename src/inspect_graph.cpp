#include "genpybind/inspect_graph.h"

#include <clang/AST/QualTypeNames.h>
#include <llvm/Support/GraphWriter.h>

using namespace genpybind;

namespace {
struct DeclContextGraphWithAnnotations {
  const DeclContextGraph *graph;
  const AnnotationStorage &annotations;

  DeclContextGraphWithAnnotations(const DeclContextGraph *graph,
                                  const AnnotationStorage &annotations)
      : graph(graph), annotations(annotations) {}

  const DeclContextGraph *operator->() const { return graph; }
};
} // namespace

namespace llvm {
template <>
struct GraphTraits<DeclContextGraphWithAnnotations>
    : public GraphTraits<const DeclContextGraph *> {
  static NodeRef getEntryNode(const DeclContextGraphWithAnnotations &graph) {
    return graph->getRoot();
  }
  static nodes_iterator
  nodes_begin(const DeclContextGraphWithAnnotations &graph) {
    return nodes_iterator(graph->begin(), &valueFromPair);
  }

  static nodes_iterator
  nodes_end(const DeclContextGraphWithAnnotations &graph) {
    return nodes_iterator(graph->end(), &valueFromPair);
  }

  static unsigned size(const DeclContextGraphWithAnnotations &graph) {
    return graph->size();
  }
};

template <>
struct DOTGraphTraits<DeclContextGraphWithAnnotations>
    : public DefaultDOTGraphTraits {
  using DefaultDOTGraphTraits::DefaultDOTGraphTraits;

  static std::string
  getNodeLabel(const DeclContextNode *node,
               const DeclContextGraphWithAnnotations &graph) {
    if (node == graph->getRoot())
      return "*";

    if (const auto *decl = dyn_cast_or_null<clang::TypeDecl>(node->getDecl())) {
      const clang::ASTContext &context = decl->getASTContext();
      const clang::QualType qual_type = context.getTypeDeclType(decl);
      return clang::TypeName::getFullyQualifiedName(
          qual_type, context, context.getPrintingPolicy());
    }

    if (const auto *decl = dyn_cast_or_null<clang::NamedDecl>(node->getDecl()))
      return decl->getQualifiedNameAsString();

    if (isa<clang::TranslationUnitDecl>(node->getDecl()))
      return "TU";

    return std::string("<") + node->getDecl()->getDeclKindName() + ">";
  }
  static std::string
  getNodeDescription(const DeclContextNode *node,
                     const DeclContextGraphWithAnnotations &graph) {
    if (const auto *decl = llvm::dyn_cast<clang::NamedDecl>(node->getDecl()))
      if (const auto *annotated = llvm::dyn_cast_or_null<AnnotatedNamedDecl>(
              graph.annotations.get(decl)))
        return annotated->spelling;
    return "";
  }

  static std::string
  getNodeAttributes(const DeclContextNode *node,
                    const DeclContextGraphWithAnnotations &graph) {
    std::string result;
    llvm::raw_string_ostream stream(result);
    stream << R"(style=")";
    if (const auto *decl = llvm::dyn_cast<clang::NamedDecl>(node->getDecl())) {
      if (const auto *annotated = llvm::dyn_cast_or_null<AnnotatedNamedDecl>(
              graph.annotations.get(decl))) {
        if (!annotated->visible.hasValue()) {
          stream << "dotted";
        } else if (!annotated->visible.getValue()) {
          stream << "dashed";
        }
      }
    }

    if (isa<clang::TranslationUnitDecl>(node->getDecl())) {
      stream << R"(,filled,bold",shape=diamond,fillcolor=Seashell2)";
    } else if (isa<clang::NamespaceDecl>(node->getDecl())) {
      stream << R"(,filled,bold",fillcolor=Seashell1)";
    } else {
      stream << R"(")";
    }
    return stream.str();
  }
};
} // namespace llvm

void genpybind::viewGraph(const DeclContextGraph *graph,
                          const AnnotationStorage &annotations,
                          const llvm::Twine &name, const llvm::Twine &title) {
  const DeclContextGraphWithAnnotations annotated_graph(graph, annotations);
  llvm::ViewGraph(annotated_graph, name, false, title);
}
