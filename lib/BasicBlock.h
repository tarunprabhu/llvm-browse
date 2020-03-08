#ifndef LLVM_BROWSE_BASIC_BLOCK_H
#define LLVM_BROWSE_BASIC_BLOCK_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include <vector>

#include "INavigable.h"
#include "IWrapper.h"
#include "Value.h"

namespace lb {

class Function;
class Instruction;
class Module;

class BasicBlock :
    public Value,
    public INavigable,
    public IWrapper<llvm::BasicBlock> {
protected:
  std::vector<Instruction*> insts;

public:
  using Iterator = decltype(insts)::const_iterator;

public:
  BasicBlock(llvm::BasicBlock& llvm_bb, Function& f, Module& module);
  virtual ~BasicBlock() = default;

  Iterator begin() const;
  Iterator end() const;
  llvm::iterator_range<Iterator> instructions() const;
  const Function& get_function() const;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == EntityKind::BasicBlock;
  }

  static bool classof(const INavigable* v) {
    return v->get_kind() == EntityKind::BasicBlock;
  }
};

} // namespace lb

#endif // LLVM_BROWSE_BASIC_BLOCK_H
