#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "lib/Errors.h"
#include "lib/Module.h"

int
main(int argc, char* argv[]) {
  bool maximize        = false;
  std::string filename = "";
  int opt              = -1;

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
    if(filename.length()) {
      std::cerr << "At most one input file may be specified\n";
      return int(ErrorCode::ExtraCommandLineArg);
    }
    filename = argv[optind];
  }

  // TODO: Start GUI here
  if(filename.length()) {
    const lb::Module module(filename);
    if(not module)
      return ErrorCode::ModuleLoad;
  }

  return 0;
}
