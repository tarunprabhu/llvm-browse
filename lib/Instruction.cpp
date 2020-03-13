#include "Instruction.h"
#include "Argument.h"
#include "BasicBlock.h"
#include "Function.h"
#include "Module.h"
#include "String.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Instruction::Instruction(const llvm::Instruction& llvm_i,
                         BasicBlock& bb,
                         Function& f,
                         Module& module) :
    Value(EntityKind::Instruction),
    INavigable(EntityKind::Instruction),
    IWrapper<llvm::Instruction>(llvm_i, module),
    parent(bb),
    di(llvm_i.getDebugLoc()) {
  if(di) {
    if(const auto* scope = dyn_cast<llvm::DIScope>(di.getScope())) {
      SourceRange defn = SourceRange(
          module.get_full_path(scope->getDirectory(), scope->getFilename()),
          di.getLine(),
          di.getCol());
      set_source_defn(defn);
      // The calls to LLVM's debug metadata intrinsics sometimes contain more
      // accurate location information than the DI nodes themselves. So if we
      // see any such calls, add whatever additional data we can
      if(const auto* call = dyn_cast<llvm::CallInst>(&llvm_i)) {
        if(const llvm::Function* callee = call->getCalledFunction()) {
          if(callee->getName() == "llvm.dbg.value") {
            if(auto* arg = dyn_cast<llvm::Argument>(call->getArgOperand(0))) {
              llvm::Metadata* md
                  = cast<llvm::MetadataAsValue>(call->getArgOperand(1))
                        ->getMetadata();
              Argument& wrapped_arg = f.get_arg(arg->getArgNo());
              wrapped_arg.set_debug_info_node(cast<llvm::DILocalVariable>(md));
              wrapped_arg.set_source_defn(defn);
            }
            // In optimized code, there may not exist an alloca corresponding
            // to a stack variable. Any instruction that sets the value
            // of some register variable could get mapped to a local variable
            // in the source. Short of creating a separate local variable
            // object, there is not much we can do with it.
          } 
        }
      }
    }
  }
}

bool
Instruction::has_source_info() const {
  return di;
}

llvm::StringRef
Instruction::get_llvm_name() const {
  return get_tag();
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

bool
Instruction::is_llvm_debug_inst() const {
  if(const auto* call = dyn_cast<llvm::CallInst>(&get_llvm()))
    if(const llvm::Function* f = call->getCalledFunction())
      if(f->getName().startswith("llvm.dbg."))
        return true;
  return false;
}

bool
Instruction::is_llvm_lifetime_inst() const {
  if(const auto* call = dyn_cast<llvm::CallInst>(&get_llvm()))
    if(const llvm::Function* f = call->getCalledFunction())
      if(f->getName().startswith("llvm.lifetime."))
        return true;
  return false;
}

BasicBlock&
Instruction::get_block() {
  return parent;
}

const BasicBlock&
Instruction::get_block() const {
  return parent;
}

const Function&
Instruction::get_function() const {
  return get_block().get_function();
}

Instruction&
Instruction::make(const llvm::Instruction& llvm_i,
                  BasicBlock& bb,
                  Function& f,
                  Module& module) {
  auto* inst = new Instruction(llvm_i, bb, f, module);
  bb.m_insts.emplace_back(inst);
  module.vmap[&llvm_i] = inst;

  return *inst;
}

} // namespace lb
