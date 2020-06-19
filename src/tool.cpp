#include "genpybind/decl_context_collector.h"
#include "genpybind/decl_context_graph.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/Version.inc>
#include <clang/Driver/Driver.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/ArgumentsAdjusters.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Program.h>

#include <memory>

using namespace genpybind;

namespace {

class GenpybindASTConsumer : public clang::ASTConsumer {
  DeclContextCollector visitor;

public:
  void HandleTranslationUnit(clang::ASTContext &context) override {
    /// Builds a graph of declaration contexts from the AST.
    DeclContextGraph graph(context.getTranslationUnitDecl());
    visitor.TraverseDecl(context.getTranslationUnitDecl());

    for (const clang::TypedefNameDecl *alias_decl : visitor.aliases) {
      static_cast<void>(alias_decl);
    }

    for (const clang::DeclContext *decl_context : visitor.decl_contexts) {
      const auto *decl = llvm::dyn_cast<clang::Decl>(decl_context);
      const auto *parent =
          llvm::dyn_cast<clang::Decl>(decl->getLexicalDeclContext());
      graph.getOrInsertNode(parent)->addChild(graph.getOrInsertNode(decl));
    }
  }
};

class GenpybindAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance & /*compiler*/,
                    llvm::StringRef /*in_file*/) {
    return std::make_unique<GenpybindASTConsumer>();
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

  llvm::cl::OptionCategory genpybind_category("Genpybind Options");

  llvm::cl::extrahelp common_help(CommonOptionsParser::HelpMessage);

  llvm::cl::opt<bool> expect_failure("xfail", llvm::cl::cat(genpybind_category),
                                    llvm::cl::desc("Reverse the exit status."),
                                    llvm::cl::init(false), llvm::cl::Hidden);

  CommonOptionsParser options_parser(argc, argv, genpybind_category);

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