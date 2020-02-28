#include "StructType.h"

#include <llvm/Support/raw_ostream.h>

#include <glibmm.h>

namespace lb {

StructType::StructType(llvm::StructType* llvm, Module& module) :
    INavigable(), IWrapper<llvm::StructType*>(llvm, module) {
  if(llvm->hasName()) {
    set_tag(llvm->getName(), "%");
  } else {
    std::string buf;
    llvm::raw_string_ostream ss(buf);
    ss << *get_llvm();
    g_critical("Cannot set tag for unnamed global: %s", buf.c_str());
  }
}

} // namespace lb
