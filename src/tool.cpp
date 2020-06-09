#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Decl.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CommandLine.h>

#include <memory>

namespace {

class GenpybindASTConsumer : public clang::ASTConsumer {
public:
  GenpybindASTConsumer(clang::ASTContext * /*context*/) {}

  void HandleTranslationUnit(clang::ASTContext & /*context*/) override {}
};

class GenpybindAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &compiler,
                    llvm::StringRef /*in_file*/) {
    return std::make_unique<GenpybindASTConsumer>(&compiler.getASTContext());
  }
};

} // namespace

int main(int argc, const char **argv) {
  using namespace ::clang::tooling;

  llvm::cl::OptionCategory genpybind_category("Genpybind Options");

  llvm::cl::extrahelp common_help(CommonOptionsParser::HelpMessage);

  CommonOptionsParser options_parser(argc, argv, genpybind_category);

  ClangTool tool(options_parser.getCompilations(),
                 options_parser.getSourcePathList());

  auto factory = newFrontendActionFactory<GenpybindAction>();

  return tool.run(factory.get());
}
