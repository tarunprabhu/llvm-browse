#include <llvm/Support/InitLLVM.h>

#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "lib/Errors.h"
#include "lib/Module.h"
#include "ui/UI.h"

int
main(int argc, char* argv[]) {
  llvm::InitLLVM(argc, argv);

  bool maximize    = false;
  std::string file = "";
  int opt          = -1;

  while((opt = getopt(argc, argv, ":x")) != -1) {
    switch(opt) {
    case 'x':
      maximize = true;
      break;
    case '?':
    default:
      std::cerr << "Unknown option: " << opt << "\n";
      return int(ErrorCode::CommandLineArg);
    }
  }

  for(; optind < argc; optind++) {
    if(file.length()) {
      std::cerr << "At most one input file may be specified\n";
      return int(ErrorCode::ExtraCommandLineArg);
    }
    file = argv[optind];
  }

  llvm::LLVMContext context;

  // TODO: Start GUI here
  if(file.length()) {
    std::unique_ptr<lb::Module> module = lb::Module::create(file, context);
    if(not module)
      return ErrorCode::ModuleLoad;
  }

  return 0;
}
