#include "genpybind/annotated_decl.h"
#include "genpybind/decl_context_graph.h"
#include "genpybind/decl_context_graph_builder.h"
#include "genpybind/decl_context_graph_processing.h"
#include "genpybind/expose.h"
#include "genpybind/inspect_graph.h"
#include "genpybind/instantiate_annotated_templates.h"
#include "genpybind/instantiate_default_arguments.h"
#include "genpybind/pragmas.h"
#include "genpybind/string_utils.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Version.inc> // IWYU pragma: keep
#include <clang/Driver/Driver.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Sema/SemaConsumer.h>
#include <clang/Tooling/ArgumentsAdjusters.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace genpybind;

namespace {

static llvm::cl::OptionCategory g_genpybind_category("Genpybind Options");

enum class InspectGraphStage {
  Visibility,
  Pruned,
};

static llvm::cl::ValuesClass getInspectGraphValues() {
  return llvm::cl::values(clEnumValN(InspectGraphStage::Visibility,
                                     "visibility",
                                     "With visibility of all nodes (unpruned)"),
                          clEnumValN(InspectGraphStage::Pruned, "pruned",
                                     "After pruning of hidden nodes"));
}

static llvm::cl::list<InspectGraphStage>
    g_inspect_graph("inspect-graph", llvm::cl::cat(g_genpybind_category),
                    llvm::cl::desc("Show the declaration context graph."),
                    getInspectGraphValues(), llvm::cl::ZeroOrMore,
                    llvm::cl::Hidden);

static llvm::cl::list<InspectGraphStage>
    g_dump_graph("dump-graph", llvm::cl::cat(g_genpybind_category),
                 llvm::cl::desc("Print the declaration context graph."),
                 getInspectGraphValues(), llvm::cl::ZeroOrMore,
                 llvm::cl::Hidden);

static llvm::cl::opt<bool>
    g_dump_ast("dump-ast", llvm::cl::cat(g_genpybind_category),
               llvm::cl::desc("Debug dump the AST after initial augmentation"),
               llvm::cl::init(false), llvm::cl::Hidden);

static const char *graphTitle(InspectGraphStage stage) {
  switch (stage) {
  case InspectGraphStage::Visibility:
    return "Declaration context graph (unpruned) with visibility of all nodes:";
  case InspectGraphStage::Pruned:
    return "Declaration context graph after pruning:";
  }
  llvm_unreachable("Unknown inspection point.");
}

static void inspectGraph(const DeclContextGraph &graph,
                         const AnnotationStorage &annotations,
                         const EffectiveVisibilityMap &visibilities,
                         const llvm::Twine &name, InspectGraphStage stage) {
  if (llvm::is_contained(g_inspect_graph, stage))
    viewGraph(&graph, annotations, "genpybind_" + name);
  if (llvm::is_contained(g_dump_graph, stage))
    printGraph(llvm::errs(), &graph, visibilities, annotations,
               graphTitle(stage));
}

static void emitQuotedArguments(llvm::raw_ostream &os,
                                llvm::ArrayRef<std::string> arguments) {
  bool first = true;
  for (llvm::StringRef arg : arguments) {
    if (!first) {
      os << ' ';
    } else {
      first = false;
    }
    if (arg.contains(' ')) {
      os << '"';
      os.write_escaped(arg);
      os << '"';
    } else {
      os << arg;
    }
  }
}

class GenpybindASTConsumer : public clang::SemaConsumer {
  AnnotationStorage annotations;
  clang::Sema *sema = nullptr;
  const genpybind::PragmaGenpybindHandler *pragma_handler;

public:
  GenpybindASTConsumer(const genpybind::PragmaGenpybindHandler *pragma_handler)
      : pragma_handler(pragma_handler) {}

  void InitializeSema(clang::Sema &sema_) override { sema = &sema_; }
  void ForgetSema() override { sema = nullptr; }

  void HandleTranslationUnit(clang::ASTContext &context) override {
    if (g_dump_ast)
      context.getTranslationUnitDecl()->dump();

    llvm::StringRef main_file = [&] {
      const auto &source_manager = context.getSourceManager();
      const auto *main_file =
          source_manager.getFileEntryForID(source_manager.getMainFileID());
      assert(main_file != nullptr);
      return main_file->getName();
    }();
    const auto module_name = [&] {
      llvm::SmallString<128> name = llvm::sys::path::stem(main_file);
      makeValidIdentifier(name);
      return name;
    }();

    DeclContextGraphBuilder builder(annotations,
                                    context.getTranslationUnitDecl());
    auto graph = builder.buildGraph();
    if (!graph.hasValue())
      return;

    auto visibilities = deriveEffectiveVisibility(*graph, annotations);

    if (reportExposeHereCycles(*graph, reachableDeclContexts(visibilities),
                               builder.getRelocatedDecls()))
      return;

    inspectGraph(*graph, annotations, visibilities, module_name,
                 InspectGraphStage::Visibility);

    auto contexts_with_visible_decls =
        declContextsWithVisibleNamedDecls(&*graph, annotations, visibilities);

    hideNamespacesBasedOnExposeInAnnotation(*graph, annotations,
                                            contexts_with_visible_decls,
                                            visibilities, module_name);

    graph = pruneGraph(*graph, contexts_with_visible_decls, visibilities);

    reportUnreachableVisibleDeclContexts(*graph, contexts_with_visible_decls,
                                         builder.getRelocatedDecls());

    inspectGraph(*graph, annotations, visibilities, module_name,
                 InspectGraphStage::Pruned);

    TranslationUnitExposer exposer(*sema, *graph, visibilities, annotations);

    llvm::outs() << "#include \"" << main_file << "\"\n"
                 << "#include <genpybind/runtime.h>\n"
                 << "#include <pybind11/pybind11.h>\n";

    if (pragma_handler != nullptr) {
      for (const std::string &include : pragma_handler->getIncludes()) {
        llvm::outs() << "#include " << include << '\n';
      }
    }
    llvm::outs() << '\n';

    exposer.emitModule(llvm::outs(), module_name);

    llvm::outs().flush();
  }
};

class GenpybindAction : public clang::ASTFrontendAction {
  std::unique_ptr<genpybind::PragmaGenpybindHandler> pragma_genpybind_handler;

public:
  bool BeginSourceFileAction(clang::CompilerInstance &compiler) override {
    pragma_genpybind_handler =
        std::make_unique<genpybind::PragmaGenpybindHandler>();
    clang::Preprocessor &preproc = compiler.getPreprocessor();
    preproc.AddPragmaHandler(pragma_genpybind_handler.get());
    return true;
  }

  void EndSourceFileAction() override {
    clang::Preprocessor &preproc = getCompilerInstance().getPreprocessor();
    preproc.RemovePragmaHandler(pragma_genpybind_handler.get());
    pragma_genpybind_handler.reset();
  }

  virtual std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance & /*compiler*/,
                    llvm::StringRef /*in_file*/) {
    std::vector<std::unique_ptr<clang::ASTConsumer>> consumers;
    consumers.push_back(
        std::make_unique<InstantiateAnnotatedTemplatesASTConsumer>());
    consumers.push_back(
        std::make_unique<InstantiateDefaultArgumentsASTConsumer>());
    consumers.push_back(
        std::make_unique<GenpybindASTConsumer>(pragma_genpybind_handler.get()));
    return std::make_unique<clang::MultiplexConsumer>(std::move(consumers));
  }
};

std::string findClangExecutable() {
#define THE_VERSION_SPECIFIC_PATH(version) "clang-" #version
  for (const char *name :
       {THE_VERSION_SPECIFIC_PATH(CLANG_VERSION_MAJOR), "clang"}) {
#undef THE_VERSION_SPECIFIC_PATH
    if (llvm::ErrorOr<std::string> path = llvm::sys::findProgramByName(name)) {
      llvm::SmallString<128> resolved;
      if (llvm::sys::fs::real_path(*path, resolved)) {
        // Failed to resolve path.
        resolved = *path;
      }
      return resolved.str();
    }
  }
  return {};
}

std::string findResourceDir() {
  // CompilerInvocation::GetResourcesPath() assumes that the tool is installed
  // alongside clang, so it cannot be used here.  Instead, assume that the
  // "clang" program matches the library that genpybind is compiled against.
  // FIXME: This is likely wrong, if there are several installed versions.
  std::string clang_path = findClangExecutable();
  if (clang_path.empty())
    return {};

  // Unfortunately CLANG_RESOURCE_DIR is not exposed via the library,
  // so it cannot be passed in here.  An alternative would be to construct a
  // driver and use the resource path it deduces.
  return clang::driver::Driver::GetResourcesPath(clang_path);
}

} // namespace

int main(int argc, const char **argv) {
  using namespace ::clang::tooling;

  llvm::cl::extrahelp common_help(CommonOptionsParser::HelpMessage);

  llvm::cl::opt<bool> expect_failure("xfail",
                                     llvm::cl::cat(g_genpybind_category),
                                     llvm::cl::desc("Reverse the exit status."),
                                     llvm::cl::init(false), llvm::cl::Hidden);

  llvm::cl::opt<bool> verbose("verbose", llvm::cl::cat(g_genpybind_category),
                              llvm::cl::desc("Use verbose output"),
                              llvm::cl::init(false));

  CommonOptionsParser options_parser(argc, argv, g_genpybind_category);

  const CompilationDatabase &compilations = options_parser.getCompilations();
  const std::vector<std::string> &source_paths =
      options_parser.getSourcePathList();

  if (verbose) {
    for (const auto &source_path : source_paths) {
      std::string abs_path = getAbsolutePath(source_path);
      std::vector<CompileCommand> compile_commands =
          compilations.getCompileCommands(abs_path);

      for (const CompileCommand &command : compile_commands) {
        llvm::errs() << "Analyzing file " << command.Filename
                     << " from within directory " << command.Directory
                     << " using command " << command.Heuristic << "\n  ";
        emitQuotedArguments(llvm::errs(), command.CommandLine);
        llvm::errs() << "\n";
      }
    }
  }

  ClangTool tool(compilations, source_paths);

  tool.appendArgumentsAdjuster(getClangStripOutputAdjuster());
  tool.appendArgumentsAdjuster(getClangStripDependencyFileAdjuster());
  tool.appendArgumentsAdjuster(getClangStripSerializeDiagnosticAdjuster());
  tool.appendArgumentsAdjuster(getClangSyntaxOnlyAdjuster());
  tool.appendArgumentsAdjuster([](const CommandLineArguments &arguments,
                                  llvm::StringRef) -> CommandLineArguments {
    // Return if there already is a -resource-dir or -resource-dir=<arg> flag
    for (llvm::StringRef arg : arguments) {
      if (arg.consume_front("-resource-dir") && (arg.empty() || arg[0] == '='))
        return arguments;
    }
    CommandLineArguments result{arguments};
    std::string resource_dir = findResourceDir();
    if (!resource_dir.empty())
      result.push_back("-resource-dir=" + resource_dir);
    return result;
  });
  // TODO: Also append -xc++, -std=c++17, -D__GENPYBIND__?
  tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-Wno-everything"));

  if (verbose) {
    tool.appendArgumentsAdjuster(
        [](const CommandLineArguments &arguments,
           llvm::StringRef filename) -> CommandLineArguments {
          llvm::errs() << "Adjusting command for file " << filename << " to\n  ";
          emitQuotedArguments(llvm::errs(), arguments);
          llvm::errs() << "\n";
          return arguments;
        });
  }

  auto factory = newFrontendActionFactory<GenpybindAction>();

  const int exit_code = tool.run(factory.get());
  return expect_failure ? (exit_code == 0) : exit_code;
}
