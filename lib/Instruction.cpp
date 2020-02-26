#include "Instruction.h"
#include "BasicBlock.h"
#include "Function.h"
#include "Module.h"
#include "String.h"

#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Instruction::Instruction(llvm::Instruction& llvm_i, Module& module) :
    Value(Value::Kind::Instruction),
    INavigable(), IWrapper<llvm::Instruction>(llvm_i, module) {
  ;
}

void
Instruction::add_operand(const SourceRange& range) {
  ops.emplace_back(range);
}

SourceRange
Instruction::get_operand(unsigned i) const {
  return ops.at(i);
}

Instruction::Iterator
Instruction::begin() const {
  return ops.cbegin();
}

Instruction::Iterator
Instruction::end() const {
  return ops.cend();
}

llvm::iterator_range<Instruction::Iterator>
Instruction::operands() const {
  return llvm::iterator_range<Instruction::Iterator>(ops);
}

const BasicBlock&
Instruction::get_block() const {
  return get_module().get(*get_llvm().getParent());
}

const Function&
Instruction::get_function() const {
  return get_block().get_function();
}

} // namespace lb
