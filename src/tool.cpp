#include "genpybind/annotated_decl.h"
#include "genpybind/annotations/literal_value.h"
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
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Version.inc> // IWYU pragma: keep
#include <clang/Driver/Driver.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Lex/Preprocessor.h>
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

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using namespace genpybind;

namespace clang {
class Sema;
}

namespace {

llvm::cl::OptionCategory g_genpybind_category("Genpybind Options");

enum class InspectGraphStage {
  Visibility,
  Pruned,
};

llvm::cl::ValuesClass getInspectGraphValues() {
  return llvm::cl::values(clEnumValN(InspectGraphStage::Visibility,
                                     "visibility",
                                     "With visibility of all nodes (unpruned)"),
                          clEnumValN(InspectGraphStage::Pruned, "pruned",
                                     "After pruning of hidden nodes"));
}

llvm::cl::list<InspectGraphStage>
    g_inspect_graph("inspect-graph", llvm::cl::cat(g_genpybind_category),
                    llvm::cl::desc("Show the declaration context graph."),
                    getInspectGraphValues(), llvm::cl::ZeroOrMore,
                    llvm::cl::Hidden);

llvm::cl::list<InspectGraphStage>
    g_dump_graph("dump-graph", llvm::cl::cat(g_genpybind_category),
                 llvm::cl::desc("Print the declaration context graph."),
                 getInspectGraphValues(), llvm::cl::ZeroOrMore,
                 llvm::cl::Hidden);

llvm::cl::opt<bool>
    g_dump_ast("dump-ast", llvm::cl::cat(g_genpybind_category),
               llvm::cl::desc("Debug dump the AST after initial augmentation"),
               llvm::cl::init(false), llvm::cl::Hidden);

struct OutputFilenameParser : public llvm::cl::parser<std::string> {
  OutputFilenameParser(llvm::cl::Option &opt) : parser(opt) {}

  bool parse(llvm::cl::Option &opt, llvm::StringRef, llvm::StringRef arg,
             std::string &value) {
    // NOTE: Absolute paths are enforced for the output files, since
    // `ClangTool::run` changes to a different working directory.
    if (arg != "-" && !llvm::sys::path::is_absolute(arg))
      return opt.error("path has to be absolute!");
    value = arg.str();
    return false;
  }

  llvm::StringRef getValueName() const override { return "absolute path"; }
};

llvm::cl::list<std::string, bool, OutputFilenameParser> g_output_files(
    "o", llvm::cl::cat(g_genpybind_category),
    llvm::cl::desc(
        "Path to the output file (\"-\" for stdout); Can be specified multiple "
        "times\nto spread the generated code over several files."),
    llvm::cl::ZeroOrMore);

llvm::cl::opt<bool> g_keep_output_files(
    "keep-output-files", llvm::cl::cat(g_genpybind_category),
    llvm::cl::desc("Don't erase the output files if compiler errors occurred."),
    llvm::cl::init(false), llvm::cl::Hidden);

const char *graphTitle(InspectGraphStage stage) {
  switch (stage) {
  case InspectGraphStage::Visibility:
    return "Declaration context graph (unpruned) with visibility of all nodes:";
  case InspectGraphStage::Pruned:
    return "Declaration context graph after pruning:";
  }
  llvm_unreachable("Unknown inspection point.");
}

void inspectGraph(const DeclContextGraph &graph,
                  const AnnotationStorage &annotations,
                  const EffectiveVisibilityMap &visibilities,
                  const llvm::Twine &name, InspectGraphStage stage) {
  if (llvm::is_contained(g_inspect_graph, stage))
    viewGraph(&graph, annotations, "genpybind_" + name);
  if (llvm::is_contained(g_dump_graph, stage)) {
    printGraph(llvm::errs(), &graph, visibilities, annotations,
               graphTitle(stage));
    if (graph.size() == 1)
      llvm::errs() << "<no children>\n";
  }
}

void emitQuotedArguments(llvm::raw_ostream &os,
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
  clang::CompilerInstance &compiler;
  const genpybind::PragmaGenpybindHandler *pragma_handler;

public:
  GenpybindASTConsumer(clang::CompilerInstance &compiler,
                       const genpybind::PragmaGenpybindHandler *pragma_handler)
      : compiler(compiler), pragma_handler(pragma_handler) {}

  // NOLINTNEXTLINE(readability-identifier-naming)
  void InitializeSema(clang::Sema &sema_) override { sema = &sema_; }
  void ForgetSema() override { sema = nullptr; }

  void HandleTranslationUnit(clang::ASTContext &context) override {
    if (g_dump_ast)
      context.getTranslationUnitDecl()->dump();

    const auto &source_manager = context.getSourceManager();
    llvm::StringRef main_file = [&] {
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
                               builder.getRelocatedDecls(), source_manager))
      return;

    inspectGraph(*graph, annotations, visibilities, module_name,
                 InspectGraphStage::Visibility);

    auto contexts_with_visible_decls = declContextsWithVisibleNamedDecls(
        *sema, &*graph, annotations, visibilities);

    hideNamespacesBasedOnExposeInAnnotation(*graph, annotations,
                                            contexts_with_visible_decls,
                                            visibilities, module_name);

    graph = pruneGraph(*graph, contexts_with_visible_decls, visibilities);

    reportUnreachableVisibleDeclContexts(*graph, contexts_with_visible_decls,
                                         builder.getRelocatedDecls(),
                                         source_manager);

    inspectGraph(*graph, annotations, visibilities, module_name,
                 InspectGraphStage::Pruned);

    std::vector<std::unique_ptr<llvm::raw_pwrite_stream>> output_streams;
    for (llvm::StringRef output_path : g_output_files) {
      bool binary = false;
      bool remove_file_on_signal = true; // not thread-safe
      llvm::StringRef base_input, extension;
      bool use_temporary = true;
      bool create_missing_directories = false;
      auto stream = compiler.createOutputFile(
          output_path, binary, remove_file_on_signal, base_input, extension,
          use_temporary, create_missing_directories);
      if (stream == nullptr)
        return;
      output_streams.push_back(std::move(stream));
    }

    if (output_streams.empty())
      return;

    TranslationUnitExposer exposer(*sema, *graph, visibilities, annotations);

    std::string includes;
    {
      llvm::raw_string_ostream stream(includes);
      stream << "#include \"" << main_file << "\"\n"
             << "#include <genpybind/runtime.h>\n"
             << "#include <pybind11/pybind11.h>\n";

      if (pragma_handler != nullptr) {
        for (const std::string &include : pragma_handler->getIncludes()) {
          stream << "#include " << include << '\n';
        }
      }
      stream << '\n';
    }

    std::vector<llvm::raw_ostream *> streams;
    for (const auto &stream : output_streams) {
      (*stream) << includes;
      streams.push_back(stream.get());
    }
    exposer.emitModule(streams, module_name);
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

  bool shouldEraseOutputFiles() override {
    return !g_keep_output_files &&
           getCompilerInstance().getDiagnostics().hasErrorOccurred();
  }

  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance & /*compiler*/,
                    llvm::StringRef /*in_file*/) override {
    std::vector<std::unique_ptr<clang::ASTConsumer>> consumers;
    consumers.push_back(
        std::make_unique<InstantiateAnnotatedTemplatesASTConsumer>());
    consumers.push_back(
        std::make_unique<InstantiateDefaultArgumentsASTConsumer>());
    consumers.push_back(std::make_unique<GenpybindASTConsumer>(
        getCompilerInstance(), pragma_genpybind_handler.get()));
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
      return resolved.str().str();
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

  // Only accept one source path, since the current implementation only allows
  // one set of output files.
  CommonOptionsParser options_parser(argc, argv, g_genpybind_category,
                                     /*OccurrencesFlag=*/llvm::cl::Required);

  if (g_output_files.empty())
    g_output_files.push_back("-");

  const CompilationDatabase &compilations = options_parser.getCompilations();
  const std::vector<std::string> &source_paths =
      options_parser.getSourcePathList();

  assert(source_paths.size() == 1);

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
  return expect_failure ? static_cast<int>(exit_code == 0) : exit_code;
}
