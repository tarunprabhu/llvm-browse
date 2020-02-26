#include "GlobalAlias.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

GlobalAlias::GlobalAlias(llvm::GlobalAlias& llvm_a, Module& module) :
    Value(Value::Kind::GlobalAlias),
    INavigable(), IWrapper<llvm::GlobalAlias>(llvm_a, module) {
  set_tag(llvm_a.getName(), "@");
}

} // namespace lb
