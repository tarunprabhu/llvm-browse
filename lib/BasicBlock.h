#ifndef LLVM_BROWSE_BASIC_BLOCK_H
#define LLVM_BROWSE_BASIC_BLOCK_H

#include <llvm/ADT/iterator_range.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/ModuleSlotTracker.h>

#include <vector>

#include "Function.h"
#include "Value.h"

namespace lb {

class Instruction;
class Module;

class BasicBlock : public Value {
protected:
  std::vector<const Instruction*> insts;

public:
  using Iterator = decltype(insts)::const_iterator;

public:
  BasicBlock(const llvm::BasicBlock& llvm_bb,
             Module& module);
  virtual ~BasicBlock() = default;

  Iterator begin() const;
  Iterator end() const;
  llvm::iterator_range<Iterator> instructions() const;
  const Function& get_function() const;
  virtual const llvm::BasicBlock& get_llvm() const override;

public:
  static bool classof(const Value* v) {
    return v->get_kind() == Value::Kind::BasicBlock;
  }

public:
  friend class Function;
};

} // namespace lb

#endif // LLVM_BROWSE_BASIC_BLOCK_H
