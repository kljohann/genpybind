#include "genpybind/inspect_graph.h"

#include "genpybind/annotated_decl.h"
#include "genpybind/decl_context_graph.h"
#include "genpybind/diagnostics.h"

#include <clang/AST/Decl.h>
#include <clang/AST/TextNodeDumper.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/DOTGraphTraits.h>
#include <llvm/Support/GraphWriter.h>
#include <llvm/Support/raw_ostream.h>

#include <string>

namespace llvm {
template <class GraphType> struct GraphTraits;
} // namespace llvm

using namespace genpybind;

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
  const EffectiveVisibilityMap &visibilities;
  const AnnotationStorage &annotations;
  using NodeRef = const DeclContextNode *;

  std::string getNodeLabel(NodeRef node) {
    const clang::Decl *decl = node->getDecl();
    std::string result = decl->getDeclKindName();
    if (const auto *named_decl = llvm::dyn_cast<clang::NamedDecl>(decl)) {
      result.append(" '");
      result.append(getNameForDisplay(decl));
      result.push_back('\'');

      if (const auto *annotated = llvm::dyn_cast_or_null<AnnotatedNamedDecl>(
              annotations.get(named_decl))) {
        const std::string spelling = !annotated->spelling.empty()
                                         ? annotated->spelling
                                         : getSpelling(named_decl);
        if (spelling != named_decl->getName()) {
          result.append(" as '");
          result.append(spelling);
          result.push_back('\'');
        }
      }
    }
    return result;
  }

  void printNodeDescription(NodeRef node) {
    auto it = visibilities.find(node->getDeclContext());
    if (it == visibilities.end()) {
      os << "unknown";
    }
    os << (it->getSecond() ? "visible" : "hidden");
  }

  void printNode(NodeRef node) {
    AddChild(getNodeLabel(node), [node, this] {
      if (node != graph->getRoot())
        printNodeDescription(node);
      for (NodeRef child : *node) {
        printNode(child);
      }
    });
  }

public:
  GraphPrinter(llvm::raw_ostream &os, const DeclContextGraph *graph,
               const EffectiveVisibilityMap &visibilities,
               const AnnotationStorage &annotations)
      : TextTreeStructure(os, false), os(os), graph(graph),
        visibilities(visibilities), annotations(annotations) {}

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
    return {graph->begin(), &valueFromPair};
  }

  static nodes_iterator
  nodes_end(const DeclContextGraphWithAnnotations &graph) {
    return {graph->end(), &valueFromPair};
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

    std::string result = getNameForDisplay(decl);
    if (result.empty())
      result = std::string("<") + decl->getDeclKindName() + ">";

    return result;
  }

  static std::string
  getNodeDescription(const DeclContextNode *node,
                     const DeclContextGraphWithAnnotations &graph) {
    if (const auto *annotated = llvm::dyn_cast_or_null<AnnotatedNamedDecl>(
            graph.annotations.get(node->getDecl())))
      return annotated->spelling;
    return "";
  }

  static std::string
  getNodeAttributes(const DeclContextNode *node,
                    const DeclContextGraphWithAnnotations &graph) {
    std::string result;
    llvm::raw_string_ostream stream(result);
    stream << R"(style=")";
    if (const auto *annotated = llvm::dyn_cast_or_null<AnnotatedNamedDecl>(
            graph.annotations.get(node->getDecl()))) {
      if (!annotated->visible.has_value()) {
        stream << "dotted";
      } else if (!annotated->visible.value()) {
        stream << "dashed";
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
                           const EffectiveVisibilityMap &visibilities,
                           const AnnotationStorage &annotations,
                           const llvm::Twine &title) {
  os << title;
  GraphPrinter(os, graph, visibilities, annotations).print();
}
