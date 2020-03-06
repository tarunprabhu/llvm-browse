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

Instruction::Instruction(llvm::Instruction& llvm_i, Function& f, Module& module) :
    Value(Value::Kind::Instruction),
    INavigable(), IWrapper<llvm::Instruction>(llvm_i, module),
    di(llvm_i.getDebugLoc()) {
  if(const llvm::DebugLoc& loc = llvm_i.getDebugLoc()) {
    if(const auto* scope = dyn_cast<llvm::DIScope>(loc.getScope())) {
      SourceRange defn = SourceRange(scope->getFilename().data(),
                                     loc.getLine(),
                                     loc.getCol());
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
            // to a stack variable. Any old instruction that sets the value
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

llvm::StringRef Instruction::get_llvm_name() const {
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

const BasicBlock&
Instruction::get_block() const {
  return get_module().get(*get_llvm().getParent());
}

const Function&
Instruction::get_function() const {
  return get_block().get_function();
}

} // namespace lb
