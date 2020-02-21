#include "Module.h"
#include "Function.h"
#include "GlobalVariable.h"
#include "Stringify.h"

#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>

namespace lb {

Module::Module(const std::string& filename) : valid(false) {
  llvm::SMDiagnostic error;
  llvm = parseIRFile(filename, error, context);
  if(llvm) {
    // The input file could be an LLVM bitcode file as opposed to
    // human-readable IR, so the source file contents can only be initialized
    // after we have a valid module
    src.set_contents(str(*llvm));

    // Do this in two passes because there may be circular references
    // in the functions
    for(llvm::Function& function : llvm->functions())
      funcs.push_back(&add<Function>(function));

    for(llvm::GlobalVariable& global : llvm->globals())
      globs.push_back(&add<GlobalVariable>(global));

    // Second pass
    for(llvm::Function& function : llvm->functions())
      get<Function>(function).init();

    valid = true;
  }
}

llvm::iterator_range<Module::FunctionIterator>
Module::functions() const {
  return llvm::iterator_range<Module::FunctionIterator>(funcs);
}

llvm::iterator_range<Module::GlobalIterator>
Module::globals() const {
  return llvm::iterator_range<Module::GlobalIterator>(globs);
}

} // namespace lb
