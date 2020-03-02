#include "GlobalVariable.h"
#include "Module.h"
#include "Logging.h"

#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

GlobalVariable::GlobalVariable(llvm::GlobalVariable& llvm_g, Module& module) :
    Value(Value::Kind::GlobalVariable),
    INavigable(), IWrapper<llvm::GlobalVariable>(llvm_g, module) {
  if(llvm_g.hasName())
    set_tag(llvm_g.getName(), "@");
  else
    critical() << "Cannot set tag for unnamed global" << llvm_g << "\n";
}

} // namespace lb
