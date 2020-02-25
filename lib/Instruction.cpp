#include "Instruction.h"
#include "Constant.h"
#include "Module.h"
#include "String.h"

#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Instruction::Instruction(const llvm::Instruction& llvm_i,
                         Module& module) :
    Value(Value::Kind::Instruction, llvm_i, module) {
}

void
Instruction::init(llvm::ModuleSlotTracker& slots) {
  for(const llvm::Value* v : get_llvm().operand_values())
    if(isa<llvm::Argument>(v) or isa<llvm::Function>(v)
       or isa<llvm::GlobalVariable>(v) or isa<llvm::Instruction>(v)
       or isa<llvm::BasicBlock>(v))
      ops.push_back(&module.get(*v));
    else if(isa<llvm::MetadataAsValue>(v))
      // FIXME: At some point, we should add some sort of flag indicating
      // that the operand is a metadata, but right now, we don't care because
      // we don't want to have to deal with it
      ;
    else if(const auto* c = dyn_cast<llvm::Constant>(v))
      ops.push_back(&module.get_or_insert<Constant>(*c));
    else
      llvm::errs() << String::str(*v) << "\n";
}

const llvm::Instruction&
Instruction::get_llvm() const {
  return llvm::cast<llvm::Instruction>(llvm);
}

} // namespace lb
