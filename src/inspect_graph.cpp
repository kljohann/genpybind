#include "genpybind/inspect_graph.h"

#include "clang/AST/TextNodeDumper.h"
#include <clang/AST/QualTypeNames.h>
#include <llvm/Support/GraphWriter.h>

using namespace genpybind;

static std::string getDeclName(const clang::Decl *decl) {
  if (const auto *type_decl = llvm::dyn_cast<clang::TypeDecl>(decl)) {
    const clang::ASTContext &context = type_decl->getASTContext();
    const clang::QualType qual_type = context.getTypeDeclType(type_decl);
    return clang::TypeName::getFullyQualifiedName(qual_type, context,
                                                  context.getPrintingPolicy());
  }

  if (const auto *named_decl = llvm::dyn_cast<clang::NamedDecl>(decl))
    return named_decl->getQualifiedNameAsString();

  return "";
}

namespace {

struct DeclContextGraphWithAnnotations {
  const DeclContextGraph *graph;
  const AnnotationStorage &annotations;

  DeclContextGraphWithAnnotations(const DeclContextGraph *graph,
                                  const AnnotationStorage &annotations)
      : graph(graph), annotations(annotations) {}

  const DeclContextGraph *operator*() const { return graph; }
  const DeclContextGraph *operator->() const { return graph; }
};

class GraphPrinter : private clang::TextTreeStructure {
  llvm::raw_ostream &os;
  const DeclContextGraph *graph;
  const AnnotationStorage &annotations;
  using NodeRef = const DeclContextNode *;

  std::string getNodeLabel(NodeRef node) {
    const clang::Decl *decl = node->getDecl();
    std::string result = decl->getDeclKindName();
    if (llvm::isa<clang::NamedDecl>(decl)) {
      result.append(" '");
      result.append(getDeclName(decl));
      result.push_back('\'');
    }
    return result;
  }

  void printNodeDescription(NodeRef node) {
    const clang::Decl *decl = node->getDecl();
    const auto *named_decl = llvm::dyn_cast<clang::NamedDecl>(decl);
    if (named_decl == nullptr)
      return;
    const auto *annotated =
        llvm::dyn_cast_or_null<AnnotatedNamedDecl>(annotations.get(named_decl));
    if (annotated == nullptr)
      return;

    if (!annotated->visible.hasValue()) {
      os << "visible(default)";
    } else {
      os << (annotated->visible.getValue() ? "visible" : "hidden");
    }

    if (!annotated->spelling.empty()) {
      os << R"(, expose_as(")" << annotated->spelling << R"x("))x";
    }
  }

  void printNode(NodeRef node) {
    AddChild(getNodeLabel(node), [node, this] {
      printNodeDescription(node);
      for (NodeRef child : *node) {
        printNode(child);
      }
    });
  }

public:
  GraphPrinter(llvm::raw_ostream &os, const DeclContextGraph *graph,
               const AnnotationStorage &annotations)
      : TextTreeStructure(os, false), os(os), graph(graph),
        annotations(annotations) {}

  void print() { printNode(graph->getRoot()); }
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

    const clang::Decl *decl = node->getDecl();

    if (isa<clang::TranslationUnitDecl>(decl))
      return "TU";

    std::string result = getDeclName(decl);
    if (result.empty())
      result = std::string("<") + decl->getDeclKindName() + ">";

    return result;
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

void genpybind::printGraph(llvm::raw_ostream &os, const DeclContextGraph *graph,
                           const AnnotationStorage &annotations,
                           const llvm::Twine &title) {
  os << title;
  GraphPrinter(os, graph, annotations).print();
}
