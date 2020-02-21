#include "GlobalVariable.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

GlobalVariable::GlobalVariable(const llvm::GlobalVariable& llvm,
                               Module& module) :
    Value(Value::Kind::GlobalVariable, llvm, module) {
  ;
}

const llvm::GlobalVariable&
GlobalVariable::get_llvm() const {
  return cast<llvm::GlobalVariable>(llvm);
}

} // namespace lb
