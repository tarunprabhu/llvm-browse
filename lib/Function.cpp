#include "Function.h"
#include "Argument.h"
#include "BasicBlock.h"
#include "Module.h"

#include <llvm/IR/InstIterator.h>
#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Function::Function(const llvm::Function& llvm_f, Module& module) :
    Value(Value::Kind::Function, llvm_f, module) {
  set_tag(llvm_f.getName(), "@");
  for(const llvm::Argument& arg : llvm_f.args())
    args.push_back(&module.add<Argument>(arg));
  for(const llvm::BasicBlock& bb : llvm_f)
    bbs.push_back(&module.add<BasicBlock>(bb));
}

Function::Iterator
Function::begin() const {
  return bbs.cbegin();
}

Function::Iterator
Function::end() const {
  return bbs.cend();
}

llvm::iterator_range<Function::Iterator>
Function::blocks() const {
  return llvm::iterator_range<Function::Iterator>(bbs);
}

const llvm::Function&
Function::get_llvm() const {
  return cast<llvm::Function>(llvm);
}

} // namespace lb
