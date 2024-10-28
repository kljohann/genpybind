// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

// We use numeric releases with optional PEP 440 suffixes:
// https://peps.python.org/pep-0440/#summary-of-permitted-suffixes-and-relative-ordering
#define GENPYBIND_VERSION_STRING "0.5.0"

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
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/FileEntry.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Version.inc> // IWYU pragma: keep
#include <clang/Config/config.h>
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
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallString.h>
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
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace genpybind;

namespace clang {
class Sema;
} // namespace clang

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

  static bool parse(llvm::cl::Option &opt, llvm::StringRef, llvm::StringRef arg,
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

llvm::cl::opt<std::string> g_module_name(
    "module-name", llvm::cl::cat(g_genpybind_category),
    llvm::cl::desc("Name of the generated Python extension module. "
                   "If not specified, defaults to a valid C identifier derived "
                   "from the main input filename."),
    llvm::cl::Optional);

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
    const llvm::StringRef main_file = [&] {
      clang::OptionalFileEntryRef main_file =
          source_manager.getFileEntryRefForID(source_manager.getMainFileID());
      assert(main_file != nullptr);
      return main_file->getName();
    }();

    if (g_module_name.empty()) {
      llvm::SmallString<128> name = llvm::sys::path::stem(main_file);
      makeValidIdentifier(name);
      g_module_name = name.str().str();
    }

    DeclContextGraphBuilder builder(annotations,
                                    context.getTranslationUnitDecl());
    auto graph = builder.buildGraph();
    if (!graph.has_value())
      return;

    auto visibilities = deriveEffectiveVisibility(*graph, annotations);

    if (reportExposeHereCycles(*graph, reachableDeclContexts(visibilities),
                               builder.getRelocatedDecls(), source_manager))
      return;

    inspectGraph(*graph, annotations, visibilities, g_module_name,
                 InspectGraphStage::Visibility);

    auto contexts_with_visible_decls = declContextsWithVisibleNamedDecls(
        *sema, &*graph, annotations, visibilities);

    hideNamespacesBasedOnExposeInAnnotation(*graph, annotations,
                                            contexts_with_visible_decls,
                                            visibilities, g_module_name);

    graph = pruneGraph(*graph, contexts_with_visible_decls, visibilities);

    reportUnreachableVisibleDeclContexts(*graph, contexts_with_visible_decls,
                                         builder.getRelocatedDecls(),
                                         source_manager);

    inspectGraph(*graph, annotations, visibilities, g_module_name,
                 InspectGraphStage::Pruned);

    std::vector<std::unique_ptr<llvm::raw_pwrite_stream>> output_streams;
    for (llvm::StringRef output_path : g_output_files) {
      bool binary = false;
      bool remove_file_on_signal = true; // not thread-safe
      bool use_temporary = true;
      bool create_missing_directories = false;
      auto stream =
          compiler.createOutputFile(output_path, binary, remove_file_on_signal,
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
             << "#include <genpybind/binding-helpers.h>\n"
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
    exposer.emitModule(streams, g_module_name);
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

  return clang::driver::Driver::GetResourcesPath(clang_path,
                                                 CLANG_RESOURCE_DIR);
}

/// Add resource dir argument based on location of clang executable, if none has
/// been specified.
clang::tooling::ArgumentsAdjuster getDefaultResourceDirAdjuster() {
  using namespace clang::tooling;
  return [](const CommandLineArguments &arguments,
            llvm::StringRef) -> CommandLineArguments {
    // Return if there already is a -resource-dir or -resource-dir=<arg> flag
    for (llvm::StringRef arg : arguments) {
      if (arg.consume_front("-resource-dir") && (arg.empty() || arg[0] == '='))
        return arguments;
    }
    CommandLineArguments result;
    result.reserve(arguments.size() + 1);
    auto positional_it = llvm::find(arguments, "--");
    std::copy(arguments.begin(), positional_it, std::back_inserter(result));
    std::string resource_dir = findResourceDir();
    if (!resource_dir.empty())
      result.push_back("-resource-dir=" + resource_dir);
    std::copy(positional_it, arguments.end(), std::back_inserter(result));
    return result;
  };
}

/// Adjust arguments to always parse as C++17 or later, as required for
/// `GENPYBIND_MANUAL`.
clang::tooling::ArgumentsAdjuster getCpp17OrLaterAdjuster() {
  using namespace clang::tooling;
  return [](const CommandLineArguments &arguments,
            llvm::StringRef) -> CommandLineArguments {
    bool uses_later_standard = false;
    llvm::StringRef language_standard_prefix = "c"; // e.g., "c" or "gnu"

    assert(!arguments.empty()); // first argument is the program name
    CommandLineArguments result{arguments.front()};
    result.reserve(arguments.size() + 2);
    result.emplace_back("-xc++");

    std::size_t idx = 1;
    const std::size_t end = arguments.size();
    for (; idx < end; ++idx) {
      llvm::StringRef arg = arguments[idx];

      if (arg == "--") {
        break;
      }

      // Skip further -x<language> arguments.
      if (arg.consume_front("-x")) {
        if (arg.empty())
          ++idx; // skip next argument too
        continue;
      }

      // Skip language standard argument, if less than 2x.
      // TODO: Take into account future changes to compiler's default values.
      if (arg.consume_front("-std=")) {
        // HACK: Simplistic check to also allow, e.g., gnu++2a.
        auto parts = arg.split("++");
        language_standard_prefix = parts.first;
        uses_later_standard = parts.second.starts_with("2");
        if (!uses_later_standard)
          continue;
      }

      result.push_back(arguments[idx]);
    }

    if (!uses_later_standard)
      result.push_back(llvm::Twine("-std=")
                           .concat(language_standard_prefix)
                           .concat("++17")
                           .str());

    // Append remaining positional arguments after `--`.
    for (; idx < end; ++idx) {
      result.push_back(arguments[idx]);
    }

    return result;
  };
}

void printVersion(llvm::raw_ostream &os) {
  os << "genpybind version " GENPYBIND_VERSION_STRING << "\n";
}

} // namespace

int main(int argc, const char **argv) {
  using namespace ::clang::tooling;

  llvm::cl::extrahelp common_help(CommonOptionsParser::HelpMessage);
  llvm::cl::SetVersionPrinter(printVersion);

  llvm::cl::opt<bool> expect_failure("xfail",
                                     llvm::cl::cat(g_genpybind_category),
                                     llvm::cl::desc("Reverse the exit status."),
                                     llvm::cl::init(false), llvm::cl::Hidden);

  llvm::cl::opt<bool> verbose("verbose", llvm::cl::cat(g_genpybind_category),
                              llvm::cl::desc("Use verbose output"),
                              llvm::cl::init(false));

  // Only accept one source path, since the current implementation only allows
  // one set of output files.
  auto expected_parser =
      CommonOptionsParser::create(argc, argv, g_genpybind_category,
                                  /*OccurrencesFlag=*/llvm::cl::Required);
  if (!expected_parser) {
    // unsupported options
    llvm::errs() << expected_parser.takeError();
    return 1;
  }
  CommonOptionsParser &options_parser = expected_parser.get();

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
  tool.appendArgumentsAdjuster(getClangSyntaxOnlyAdjuster());
  tool.appendArgumentsAdjuster(getDefaultResourceDirAdjuster());
  tool.appendArgumentsAdjuster(getCpp17OrLaterAdjuster());
  tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-Wno-everything"));
  tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-D__GENPYBIND__"));

  if (verbose) {
    tool.appendArgumentsAdjuster(
        [](const CommandLineArguments &arguments,
           llvm::StringRef filename) -> CommandLineArguments {
          llvm::errs() << "Adjusting command for file " << filename
                       << " to\n  ";
          emitQuotedArguments(llvm::errs(), arguments);
          llvm::errs() << "\n";
          return arguments;
        });
  }

  auto factory = newFrontendActionFactory<GenpybindAction>();

  const int exit_code = tool.run(factory.get());
  return expect_failure ? static_cast<int>(exit_code == 0) : exit_code;
}
