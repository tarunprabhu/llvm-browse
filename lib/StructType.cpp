#include "StructType.h"
#include "Logging.h"

#include <llvm/Support/raw_ostream.h>

namespace lb {

StructType::StructType(llvm::StructType* llvm, Module& module) :
    INavigable(), IWrapper<llvm::StructType*>(llvm, module) {
  if(llvm->hasName())
    set_tag(llvm->getName(), "%");
  else
    critical() << "Cannot set tag for unnamed global: " << *get_llvm() << "\n";
}

} // namespace lb
