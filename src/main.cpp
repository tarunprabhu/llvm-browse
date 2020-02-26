#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "App.h"
#include "lib/Errors.h"
#include "lib/Module.h"
#include "ui/UI.h"

enum class SanityCheck {
  None,
  TopLevel,
  NoMeta,
  All,
};

static llvm::cl::opt<bool>
    lb_maximize("maximize",
                llvm::cl::desc("Start the window maximized"),
                llvm::cl::init(false));
static llvm::cl::alias
    lb_alias_maximize("x",
                      llvm::cl::desc("Start the window maximized"),
                      llvm::cl::aliasopt(lb_maximize));
static llvm::cl::opt<SanityCheck> lb_check(
    "check",
    llvm::cl::desc("Run sanity check only. <input file> must be provided"),
    llvm::cl::values(
        clEnumValN(SanityCheck::TopLevel,
                   "top",
                   "Check only top-level entity locations, including structs"),
        clEnumValN(SanityCheck::NoMeta,
                   "nometa",
                   "Check everything except metadata"),
        clEnumValN(SanityCheck::All,
                   "all",
                   "Check everything including metadata")),
    llvm::cl::init(SanityCheck::None));

static llvm::cl::opt<std::string> lb_file(llvm::cl::Positional,
                                          llvm::cl::desc("<input file>"),
                                          llvm::cl::init(""));

static void parse_commandline_args(int argc, char** argv) {
  // FIXME: This is really ridiculous. I shouldn't have to remove all the
  // extra arguments that LLVM adds by default this way, should I?
  const std::set<llvm::StringRef> opts
      = {"maximize", "x", "check", "help", "help-hidden"};
  for(auto& it : llvm::cl::getRegisteredOptions())
    if(opts.find(it.first()) == opts.end())
      it.second->removeArgument();

  llvm::cl::HideUnrelatedOptions(llvm::cl::GeneralCategory);
  llvm::cl::ParseCommandLineOptions(argc, argv, "GUI to browse LLVM IR");
}

int
main(int argc, char* argv[]) {
  lb::App app;

  parse_commandline_args(argc, argv);
  if(lb_check != SanityCheck::None) {
    if(not lb_file.length()) {
      g_error("Input file must be provided when running sanity checks");
      return ErrorCode::CommandLineArg;
    }

    if(not app.open_file(lb_file)) {
      g_error("Error loading LLVM module");
      return ErrorCode::ModuleLoad;
    }
    
    switch(lb_check) {
    case SanityCheck::TopLevel:
      if(not app.get_module().check_top_level()) {
        g_error("Sanity check failed");
        return ErrorCode::Sanity;
      }
      break;
    case SanityCheck::NoMeta:
      if(not app.get_module().check_all(false)) {
        g_error("Sanity check failed");
        return ErrorCode::Sanity;
      }
      break;
    case SanityCheck::All:
      if(not app.get_module().check_all(true)) {
        g_error("Sanity check failed");
        return ErrorCode::Sanity;
      }
      break;
    default:
      g_error("Unknown sanity check");
      return ErrorCode::Internal;
    }
  } else {
    // TODO: Start GUI here
    g_warning("GUI not implemented");
  }

  return 0;
}
