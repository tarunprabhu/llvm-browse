#include "BasicBlock.h"
#include "Instruction.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

BasicBlock::BasicBlock(const llvm::BasicBlock& llvm_bb,
                       Module& module) :
    Value(Value::Kind::BasicBlock, llvm_bb, module) {
  for(const llvm::Instruction& inst : llvm_bb)
    insts.push_back(&module.add<Instruction>(inst));
}

void
BasicBlock::init(llvm::ModuleSlotTracker& slots) {
  const llvm::BasicBlock& bb = get_llvm();
  if(bb.hasName())
    set_tag(bb.getName(), "%");
  else 
    set_tag(slots.getLocalSlot(&bb));
  for(const llvm::Instruction& inst : bb)
    module.get<Instruction>(inst).init(slots);
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
  return module.get<Function>(*get_llvm().getParent());
}

const llvm::BasicBlock&
BasicBlock::get_llvm() const {
  return cast<llvm::BasicBlock>(llvm);
}

} // namespace lb
