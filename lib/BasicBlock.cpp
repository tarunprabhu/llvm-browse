#include "BasicBlock.h"
#include "Function.h"
#include "Instruction.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

BasicBlock::BasicBlock(const llvm::BasicBlock& llvm_bb,
                       Function& f,
                       Module& module) :
    Value(EntityKind::BasicBlock),
    INavigable(EntityKind::BasicBlock),
    IWrapper<llvm::BasicBlock>(llvm_bb, module),
    parent(f) {
  for(const llvm::Instruction& inst : llvm_bb)
    Instruction::make(inst, *this, f, module);
}

BasicBlock::InstIterator
BasicBlock::begin() const {
  return InstIterator(m_insts.begin());
}

BasicBlock::InstIterator
BasicBlock::end() const {
  return InstIterator(m_insts.end());
}

llvm::iterator_range<BasicBlock::InstIterator>
BasicBlock::instructions() const {
  return llvm::iterator_range<InstIterator>(InstIterator(m_insts.begin()),
                                            InstIterator(m_insts.end()));
}

Function&
BasicBlock::get_function() {
  return parent;
}

const Function&
BasicBlock::get_function() const {
  return parent;
}

bool
BasicBlock::has_source_info() const {
  return get_function().has_source_info();
}

BasicBlock&
BasicBlock::make(const llvm::BasicBlock& llvm_bb, Function& f, Module& module) {
  auto* bb = new BasicBlock(llvm_bb, f, module);
  f.m_blocks.emplace_back(bb);
  module.vmap[&llvm_bb] = bb;

  return *bb;
}

} // namespace lb
