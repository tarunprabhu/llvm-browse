#include "GlobalAlias.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

GlobalAlias::GlobalAlias(const llvm::GlobalAlias& llvm_a, Module& module) :
    Value(Value::Kind::GlobalAlias, llvm_a, module) {
  set_tag(llvm_a.getName(), "@");
}

const llvm::GlobalAlias&
GlobalAlias::get_llvm() const {
  return cast<llvm::GlobalAlias>(llvm);
}

} // namespace lb
