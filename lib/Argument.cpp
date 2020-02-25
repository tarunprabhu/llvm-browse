#include "Argument.h"
#include "Function.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Argument::Argument(const llvm::Argument& arg,
                   Module& module) :
    Value(Value::Kind::Argument, arg, module) {
  ;
}

void
Argument::init(llvm::ModuleSlotTracker& slots) {
  set_tag(slots.getLocalSlot(&get_llvm()));
}

const Function&
Argument::get_function() const {
  return module.get<Function>(*get_llvm().getParent());
}

const llvm::Argument&
Argument::get_llvm() const {
  return cast<llvm::Argument>(llvm);
}

} // namespace lb
