#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/Version.inc>
#include <clang/Driver/Driver.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/MultiplexConsumer.h>
#include <clang/Tooling/ArgumentsAdjusters.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Program.h>

#include <memory>

#include "genpybind/decl_context_graph_builder.h"
#include "genpybind/inspect_graph.h"
#include "genpybind/instantiate_alias_targets.h"

using namespace genpybind;

static llvm::cl::OptionCategory g_genpybind_category("Genpybind Options");

static llvm::cl::opt<bool>
    g_inspect_graph("inspect-graph", llvm::cl::cat(g_genpybind_category),
                    llvm::cl::desc("Show the declaration context graph."),
                    llvm::cl::init(false), llvm::cl::Hidden);

namespace {

class GenpybindASTConsumer : public clang::ASTConsumer {
  AnnotationStorage annotations;

public:
  void HandleTranslationUnit(clang::ASTContext &context) override {
    DeclContextGraphBuilder builder(annotations,
                                    context.getTranslationUnitDecl());
    if (!builder.buildGraph())
      return;

    if (!builder.propagateVisibility())
      return;

    if (g_inspect_graph)
      viewGraph(&builder.getGraph(), annotations, "DeclContextGraph");
  }
};

class GenpybindAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance & /*compiler*/,
                    llvm::StringRef /*in_file*/) {
    std::vector<std::unique_ptr<clang::ASTConsumer>> consumers;
    consumers.push_back(std::make_unique<InstantiateAliasTargetsASTConsumer>());
    consumers.push_back(std::make_unique<GenpybindASTConsumer>());
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

  CommonOptionsParser options_parser(argc, argv, g_genpybind_category);

  ClangTool tool(options_parser.getCompilations(),
                 options_parser.getSourcePathList());

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

  auto factory = newFrontendActionFactory<GenpybindAction>();

  const int exit_code = tool.run(factory.get());
  return expect_failure ? (exit_code == 0) : exit_code;
}
