#include "Use.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Use::Use(Offset begin,
         Offset end,
         const INavigable& used,
         const Instruction* inst) :
    range(begin, end), used(&used), inst(inst) {
  ;
}

Offset
Use::get_begin() const {
  return range.get_begin();
}

Offset
Use::get_end() const {
  return range.get_end();
}

const INavigable*
Use::get_used() const {
  return used;
}

const Instruction*
Use::get_instruction() const {
  return inst;
}

Use&
Use::make(Offset begin,
          Offset end,
          const INavigable& used,
          Module& module,
          const Instruction* inst) {
  auto* use = new Use(begin, end, used, inst);
  module.uses.emplace_back(use);

  return *use;
}

} // namespace lb