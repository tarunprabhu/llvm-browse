#include "Argument.h"
#include "Function.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Argument::Argument(llvm::Argument& arg, Module& module) :
    Value(Value::Kind::Argument),
    INavigable(), IWrapper<llvm::Argument>(arg, module) {
  ;
}

const Function&
Argument::get_function() const {
  return get_module().get(*get_llvm().getParent());
}

} // namespace lb
