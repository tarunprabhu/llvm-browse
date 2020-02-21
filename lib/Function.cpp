#include "Function.h"
#include "Argument.h"
#include "BasicBlock.h"
#include "Module.h"

#include <llvm/Support/Casting.h>

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace lb {

Function::Function(const llvm::Function& function, Module& module) :
    Value(Value::Kind::Function, function, module) {
  for(const llvm::Argument& arg : get_llvm().args())
    args.push_back(&module.add<Argument>(arg));
  for(const llvm::BasicBlock& bb : get_llvm())
    bbs.push_back(&module.add<BasicBlock>(bb));
}

void
Function::init() {
  for(const llvm::BasicBlock& bb : get_llvm())
    module.get<BasicBlock>(bb).init();
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
