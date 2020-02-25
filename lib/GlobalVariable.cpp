#include "GlobalVariable.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

GlobalVariable::GlobalVariable(const llvm::GlobalVariable& llvm_g,
                               Module& module) :
    Value(Value::Kind::GlobalVariable, llvm_g, module) {
  if(llvm_g.hasName())
    set_tag(llvm_g.getName(), "@");
  else
    llvm::errs() << "UNSUPPORTED: Unnamed globals: " << llvm_g << "\n";
}

void
GlobalVariable::init(llvm::ModuleSlotTracker& slots) {
  ;
}

const llvm::GlobalVariable&
GlobalVariable::get_llvm() const {
  return cast<llvm::GlobalVariable>(llvm);
}

} // namespace lb
