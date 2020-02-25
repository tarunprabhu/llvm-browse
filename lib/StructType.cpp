#include "StructType.h"

#include <llvm/Support/raw_ostream.h>

namespace lb {

StructType::StructType(llvm::StructType* llvm, Module& module) :
    Navigable(), llvm(llvm), module(module) {
  set_tag(llvm->getName(), "%");
}

const Module&
StructType::get_module() const {
  return module;
}

llvm::StructType*
StructType::get_llvm() const {
  return llvm;
}

} // namespace lb
