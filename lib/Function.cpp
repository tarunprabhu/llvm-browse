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

Function::Function(llvm::Function& llvm_f, Module& module) :
    Value(Value::Kind::Function),
    INavigable(), IWrapper<llvm::Function>(llvm_f, module) {
  set_tag(llvm_f.getName(), "@");
  for(llvm::Argument& arg : llvm_f.args())
    args.push_back(&get_module().add(arg));
  for(llvm::BasicBlock& bb : llvm_f)
    bbs.push_back(&get_module().add(bb));
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

Function::ArgIterator
Function::arg_begin() const {
  return args.cbegin();
}

Function::ArgIterator
Function::arg_end() const {
  return args.cend();
}

llvm::iterator_range<Function::ArgIterator>
Function::arguments() const {
  return llvm::iterator_range<Function::ArgIterator>(args);
}

} // namespace lb
