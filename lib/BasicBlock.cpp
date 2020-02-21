#include "BasicBlock.h"
#include "Instruction.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

BasicBlock::BasicBlock(const llvm::BasicBlock& bb, Module& module) :
    Value(Value::Kind::BasicBlock, bb, module) {
  for(const llvm::Instruction& inst : bb)
    insts.push_back(&module.add<Instruction>(inst));
}

void
BasicBlock::init() {
  for(const llvm::Instruction& inst : get_llvm())
    module.get<Instruction>(inst).init();
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
