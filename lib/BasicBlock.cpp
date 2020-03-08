#include "BasicBlock.h"
#include "Function.h"
#include "Instruction.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

BasicBlock::BasicBlock(llvm::BasicBlock& llvm_bb, Function& f, Module& module) :
    Value(EntityKind::BasicBlock),
    INavigable(EntityKind::BasicBlock), IWrapper<llvm::BasicBlock>(llvm_bb,
                                                                   module) {
  for(llvm::Instruction& inst : llvm_bb)
    insts.push_back(&get_module().add(inst, f));
}

BasicBlock::Iterator
BasicBlock::begin() const {
  return insts.cbegin();
}

BasicBlock::Iterator
BasicBlock::end() const {
  return insts.cend();
}

llvm::iterator_range<BasicBlock::Iterator>
BasicBlock::instructions() const {
  return llvm::iterator_range<BasicBlock::Iterator>(insts);
}

const Function&
BasicBlock::get_function() const {
  return get_module().get(*get_llvm().getParent());
}

// const llvm::BasicBlock&
// BasicBlock::get_llvm() const {
//   return cast<llvm::BasicBlock>(llvm);
// }

} // namespace lb
