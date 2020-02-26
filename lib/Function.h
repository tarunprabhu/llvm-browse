#ifndef LLVM_BROWSE_FUNCTION_H
#define LLVM_BROWSE_FUNCTION_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include <vector>

#include "INavigable.h"
#include "IWrapper.h"
#include "Value.h"

namespace lb {

class Argument;
class BasicBlock;
class Module;

class Function :
    public Value,
    public INavigable,
    public IWrapper<llvm::Function> {
protected:
  std::vector<const Argument*> args;
  std::vector<const BasicBlock*> bbs;

public:
  using ArgIterator = decltype(args)::const_iterator;
  using Iterator    = decltype(bbs)::const_iterator;

public:
  Function(llvm::Function& llvm_f, Module& module);
  virtual ~Function() = default;

  Iterator begin() const;
  Iterator end() const;
  llvm::iterator_range<Iterator> blocks() const;
  ArgIterator arg_begin() const;
  ArgIterator arg_end() const;
  llvm::iterator_range<ArgIterator> arguments() const;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == Value::Kind::Function;
  }
};

} // namespace lb

#endif // LLVM_BROWSE_FUNCTION_H
