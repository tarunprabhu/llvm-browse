#include "Constant.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Constant::Constant(const llvm::Constant& c, Module& module) :
    Value(Value::Kind::Constant, c, module) {
  ;
}

void
Constant::init(llvm::ModuleSlotTracker& slots) {
  ;
}

const llvm::Constant&
Constant::get_llvm() const {
  return cast<llvm::Constant>(llvm);
}

} // namespace lb
