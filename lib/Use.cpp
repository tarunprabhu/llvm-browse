#include "Use.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Use::Use(uint64_t begin,
         uint64_t end,
         const INavigable& used,
         const Instruction* inst) :
    range(begin, end),
    used(&used), inst(inst) {
  ;
}

uint64_t
Use::get_begin() const {
  return range.get_begin();
}

uint64_t
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

} // namespace lb